import org.apache.commons.cli.*;
import java.net.URI;
import java.io.File;
import java.io.FileInputStream;
import org.apache.hadoop.io.IOUtils;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FSDataOutputStream;

public class hdp_push {
    public static void main(String args[]) throws Exception
    {
        CommandLineParser parser = new BasicParser( );
        Options options = new Options( );
        options.addOption("h", "help", false, "Print this usage information");
        options.addOption("v", "verbose", false, "Print out VERBOSE information" );
        //options.addOption("s", "srcp", true, "source parent directory, default to current dir");
        //options.addOption("d", "dstp", true, "distination parent directory, must be provided");
        options.addOption("a", "appf", true, "append to target file, specify local file name");
        options.addOption("s", "apps", true, "append string to target file");
        options.addOption("m", "merge", true, "merge two files based on both parent directory");
        options.addOption("f", "file", true, "target file, must be provided");
        
        String tgfile = "";
        String appstr = "";
        String appfile = "";
        String mergefile = "";
        // Parse the program arguments
        try {
            CommandLine cmdl = parser.parse(options, args, true);
            if (cmdl.hasOption("h")) { usage(); }
            if (cmdl.hasOption("f")) { 
                tgfile = cmdl.getOptionValue('f'); 
            } else {
                usage();
            }

            if (cmdl.hasOption("s")) { 
                appstr = cmdl.getOptionValue('s'); 
                System.out.printf("append string to file '%s'\n", tgfile);
            }

            if (cmdl.hasOption("a")) { 
                if (appstr != "") { usage(); }
                appfile = cmdl.getOptionValue('a'); 
                System.out.printf("append file '%s' to '%s'\n", appfile, tgfile);
            }

            if (cmdl.hasOption("m")) { 
                if (appstr != "" || appfile != "") { usage(); }
                mergefile = cmdl.getOptionValue('m'); 
                System.out.printf("merge file '%s' to '%s'\n", mergefile, tgfile);
            }

        } catch (Exception ex) {
            usage();
        }

        Configuration conf = new Configuration();

        //System.out.printf("do real work\n");
        if (appstr != "") {
            FileSystem fs = FileSystem.get(URI.create(tgfile), conf);

            FSDataOutputStream out;
            if (!fs.exists(new Path(tgfile))) {
                out = fs.create(new Path(tgfile));
            } else {
                out = fs.append(new Path(tgfile));
            }

            try { 
                StringBuilder sb = new StringBuilder();
                sb.append(appstr);
                sb.append('\n');

                out.writeBytes(sb.toString());
            } finally {
                out.sync();
                out.close();
            }
        } 
        else if (appfile != "") {
            FileSystem fs = FileSystem.get(URI.create(tgfile), conf);

            FSDataOutputStream out;
            if (!fs.exists(new Path(tgfile))) {
                out = fs.create(new Path(tgfile));
            } else {
                out = fs.append(new Path(tgfile));
            }
            append_file(appfile, out, fs);
        } 
        else if (mergefile != "") {
            String[] tmpfiles = mergefile.split(",");
            String[] dstfiles = tgfile.split(",");
            int filescnt = tmpfiles.length <= dstfiles.length ? tmpfiles.length : dstfiles.length;
            for (int i=0 ; i < filescnt ; ++i) {
                System.out.printf("from %s to %s\n", tmpfiles[i], dstfiles[i]);
                merge_path(tmpfiles[i], dstfiles[i], conf);
            }
        } else {
            System.err.println("Err: no string/append/merge file provided");
        }
    }

    //private static boolean check_exist(String path, FileSystem fs) throws Exception
    //{
        //return fs.exists(new Path(path));
    //}

    private static void append_file(String lcpath, FSDataOutputStream out, FileSystem fs) throws Exception
    {
        FileInputStream inf = new FileInputStream(lcpath); 
        IOUtils.copyBytes(inf, out, 4096, true);
        inf.close();
    }

    private static void merge_file(String lcpath, FSDataOutputStream out, FileSystem fs) throws Exception
    {
        //FileStatus fstat = fs.getFileStatus(Path f);
        long dpos = out.getPos();
        File lf = new File(lcpath);
        long llen = lf.length();

        if (llen <= dpos) {
            System.err.printf("no merge, local file size: %d <= remote file size: %d\n", llen, dpos);
        }

        FileInputStream inf = new FileInputStream(lcpath); 
        inf.skip(dpos);
        IOUtils.copyBytes(inf, out, 4096, true);
        inf.close();
    }

    private static void merge_path(String lcpath, String dstpath, Configuration conf) throws Exception
    {
        FileSystem fs = FileSystem.get(URI.create(dstpath), conf);

        FSDataOutputStream out;
        if (!fs.exists(new Path(dstpath))) {
            out = fs.create(new Path(dstpath));
        } else {
            out = fs.append(new Path(dstpath));
        }

        merge_file(lcpath, out, fs);
    }

    private static void usage()
    {
        String usages = "Usage: exe [options] <-s ..> | <-a ..> | <-m ..>\n"+
            "\t-h    --help       Print this usage information\n"+
            "\t-v    --verbose    Print out VERBOSE information\n"+
            "\t-f    --file       target operated file, must be provided\n"+
            "\t-s    --apps       append string to one target file\n"+
            "\t-a    --appf       append to one target file from local file\n"+
            "\t-m    --merge      merge two files with matched src/dst num, comma(,) separated files path\n\n"+
            "Example:\n\tjava hdp-push -m /tmp/local-tmp -f /tmp/file.txt\t# merge /tmp/local-tmp to /tmp/file.txt\n"+
                    "\tjava hdp-push -m /tmp/local-tmp,/tmp/tmp2 -f /tmp/file.txt,/tmp/tmp2\t# merge 2 files"
                ;
        System.out.println(usages);
        System.exit(1);
    }
}
