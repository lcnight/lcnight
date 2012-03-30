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
        FileSystem fs = FileSystem.get(URI.create(tgfile), conf);

        FSDataOutputStream out;
        if (!fs.exists(new Path(tgfile))) {
            out = fs.create(new Path(tgfile));
        } else {
            out = fs.append(new Path(tgfile));
        }

        if (appstr != "") {
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
            append_file(appfile, out, fs);
        } 
        else if (mergefile != "") {
            merge_file(mergefile, out, fs);
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
            System.exit(0);
        }

        FileInputStream inf = new FileInputStream(lcpath); 
        inf.skip(dpos);
        IOUtils.copyBytes(inf, out, 4096, true);
        inf.close();
    }

    private static void usage()
    {
        String usages = "Usage: exe [options] <-s ..> | <-a ..> | <-m ..>\n"+
            "\t-h    --help       Print this usage information\n"+
            "\t-v    --verbose    Print out VERBOSE information\n"+
            "\t-f    --file       target operated file, must be provided\n"+
            "\t-s    --apps       append string to target file\n"+
            "\t-a    --appf       append to target file, specify local file name\n"+
            "\t-m    --merge      merge two files based on both parent directory\n\n"+
            "Example:\n\texe -s local-tmp -d /tmp/input -m file.txt\t# merge file.txt from local-tmp/ to /tmp/input/"
                ;
        System.out.println(usages);
        System.exit(1);
    }
}
