import java.util.*;
import java.io.File;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.FileInputStream;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.conf.Configured;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.util.Tool;
import org.apache.hadoop.util.ToolRunner;
import org.apache.hadoop.util.ReflectionUtils;
import org.apache.hadoop.io.IOUtils;
import org.apache.hadoop.io.compress.CodecPool;
import org.apache.hadoop.io.compress.Compressor;
import org.apache.hadoop.io.compress.CompressionCodec;

import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.recipes.lock.*;


public class dfsMerge extends Configured implements Tool {

    private HashMap<String, String> codecClass = new HashMap<String, String>();
    private HashMap<String, String> codecSuffix = new HashMap<String, String>();

    public dfsMerge() {
        codecClass.put("default", "org.apache.hadoop.io.compress.BZip2Codec");
        codecClass.put("deflate", "org.apache.hadoop.io.compress.DefaultCodec");
        codecClass.put("gzip", "org.apache.hadoop.io.compress.GzipCodec");
        codecClass.put("bzip2", "org.apache.hadoop.io.compress.BZip2Codec");
        codecClass.put("lzo", "com.hadoop.compression.lzo.lzopCodec");

        codecSuffix.put("default", ".bz2");
        codecSuffix.put("deflate", ".deflate");
        codecSuffix.put("gzip", ".gz");
        codecSuffix.put("bzip2", ".bz2");
        codecSuffix.put("lzo", ".lzo");
    }

    enum SYNCMODE { MERGE, APPEND };

    public int run(String[] args) throws Exception {
        if (args.length == 0) Usage(); 

        boolean verbose = false;
        boolean compress = false;
        String compresstype = "bzip2";
        String[] srcfiles = null;
        String[] dstfiles = null;
        String dstdir = null;
        String zkServer = null;
        SYNCMODE smode = SYNCMODE.MERGE;

        for (int i=0 ; i < args.length ; ++i) {
            char c = args[i].charAt(0);
            if (c != '-') { Usage(); }
            c = args[i].charAt(1);
            switch (c)
            {
                case 's' :
                    srcfiles = args[++i].split(","); break;
                case 'f' :
                    dstfiles = args[++i].split(","); break;
                case 'd' :
                    dstdir = args[++i]; 
                    if (dstdir.endsWith("/")) {
                        dstdir = dstdir.substring(0, dstdir.length()-1);
                    }
                    break;
                case 'c':
                    compress = true; break;
                case 't' :
                    compresstype = args[++i]; break;
                case 'm' :
                    if (args[++i].toLowerCase().equals("append")) 
                    { smode = SYNCMODE.APPEND; } 
                    break;
                case 'x' :
                    zkServer = args[++i]; break;
                case 'v':
                    verbose = true; break;
                case 'h':
                    Usage();
                default :
                    System.err.printf("unknown option [ %c ]\n", c);
                    Usage(); break;
            }  /* end of switch */
        }

        // build dst files (comma separated)
        if ((dstdir != null && dstfiles != null) || (dstdir == null && dstfiles == null)) {
            System.err.println("[-f] and [-d] options cannot be used or not used at the same time\n");
            Usage();
        } else if (dstdir != null && dstfiles == null) {
            dstfiles = new String[srcfiles.length];
            for (int i = 0 ; i < srcfiles.length ; ++i) {
                File f = new File(srcfiles[i]);
                dstfiles[i] = String.format("%s/%s", dstdir, f.getName());
            }
        } else {
        }

        //MiscUtil.printArray(srcfiles);
        //MiscUtil.printArray(dstfiles);

        Configuration conf = getConf();

        // check if in compress mode
        CompressionCodec codec = null;
        Compressor compressor = null;
        if (compress) {
            // create codec
            String comCodecName = codecClass.get(compresstype);
            if (verbose) {
                System.out.printf("----- <summary> running in [%s] compression mode -----\n"+
                        "compression class: %s ,",compresstype, comCodecName);
            }
            Class<?> codecClass = Class.forName(comCodecName);
            codec = (CompressionCodec) ReflectionUtils.newInstance(codecClass, conf);

            // reform file path with correct suffix
            String comCodecSuffix = codecSuffix.get(compresstype);
            if (verbose) {
                System.out.printf(" suffix: %s\n", comCodecSuffix);
            }
            for (int i=0 ; i < dstfiles.length ; ++i) {
               dstfiles[i] = dstfiles[i] + comCodecSuffix;
            }
        }

        FileSystem fs = FileSystem.get(conf);
        int srccnt = srcfiles.length;
        int dstcnt = dstfiles.length;
        int mcnt = srccnt < dstcnt ? srccnt : dstcnt;

        ZooKeeper zks = null;
        WriteLock wl = null;
        if (zkServer != null) {
            zks = new ZooKeeper(zkServer, 5000, new zkWatcher());
            wl = new WriteLock(zks, "", null);
        }

        System.out.printf("[%s] --- do main sync ---\n", DateUtil.getCurStr());
        for (int i = 0 ; i < mcnt ; ++i) {
            File inf = new File(srcfiles[i]);
            Path outpath = new Path(dstfiles[i]);
            if (!inf.exists()) {
                System.err.printf("not exists and skip src [%s]\n", srcfiles[i]);
                continue;
            }

            if (verbose) {
                System.out.printf("[%s] merge: %s => %s\n", DateUtil.getCurStr(), 
                        srcfiles[i], dstfiles[i]);
            }
            if (zkServer != null) { // in x mode
                wl.setDir(outpath.toString());
                while (!wl.lock()) { Thread.sleep(3000); }
                if(verbose) System.out.printf("lock path: %s\n", outpath.toString());
            }
            //InputStream instream = new BufferedInputStream(new FileInputStream(srcfiles[i]));
            InputStream instream = new FileInputStream(srcfiles[i]);
            OutputStream outstream = null;
            if (fs.exists(outpath)) {
                if (verbose) System.out.printf("exists: %s\n", outpath.toString());
                FileStatus outf = fs.getFileStatus(new Path(dstfiles[i]));
                long dstlen = outf.getLen();
                long srclen = inf.length();
                if (smode == SYNCMODE.MERGE) {
                    if (srclen <= dstlen) {
                        System.err.printf("src [%s] length %d <= dst [%s] length %d\n", 
                                srcfiles[i], srclen, dstfiles[i], dstlen);
                        continue;
                    }
                    instream.skip(dstlen);
                }
                outstream = fs.append(outpath);
            } else {
                // real type is <FSDataOutputStream>
                outstream = fs.create(outpath);
            }

            if (compress) {
                compressor = CodecPool.getCompressor(codec);
                outstream = codec.createOutputStream(outstream, compressor);
            }

            IOUtils.copyBytes(instream, outstream, 4096, true);
            if(verbose) System.out.printf("sync: %s\n", srcfiles[i]);

            CodecPool.returnCompressor(compressor);
            if (zkServer != null) {
                wl.unlock();
                if(verbose) System.out.printf("unlock path: %s\n", outpath.toString());
            }
        }

        System.out.printf("[%s] process all files done\n", DateUtil.getCurStr());

        if (srccnt != dstcnt) {
            System.err.printf("[WARN] src files count [%d] != dst files count [%d]\n", 
                    srccnt, dstcnt);
        }
        return 0;
    }

    private void mergeFile(FileSystem fs, InputStream in, OutputStream out) throws Exception {
        IOUtils.copyBytes(in, out, 4096, true);
    }

    public static void main(String[] args) throws Exception
    {
        int ret = ToolRunner.run(new Configuration(), new dfsMerge(), args);
        System.exit(ret);
    }

    public static void Usage() {
        //try { 
            //throw new Exception("xx");
        //} catch (Exception e) {
            //e.printStackTrace();
        //} 
        
        System.out.printf("Usage: <jarClass> [options]\n" +
                "options: \n" +
                "\t-s       src, one file or batch files (comma seperated) need to merge to dfs\n" +
                "\t-f       dst, one file or batch files (comma seperated) match the target for src files\n" +
                "\t-d       dst dir, one directory uri specify target dfs, cannot appear with [-f] options\n" +
                "\t-m       specify run in [append|merge] mode, default in <merge> mode\n" +
                "\t-c       merge log file running in compression mode\n" +
                "\t-t       explicitly set compression type, default is [bzip2], [-c] option must be set\n" +
                "\t-x       using distribution write lock, set zookeeper server\n" +
                "\t-v       run in VERBOSE mode\n" +
                "\t-h       show help and exit\n\n" +
                "example: java dfsMerge -x localhost:2181/lock -s ./xx.log -f /xx/1.log\n\n"
                );
        System.exit(0);
    }
}
