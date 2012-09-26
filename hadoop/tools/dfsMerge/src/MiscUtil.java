//package core;

import java.io.IOException;
import java.util.ArrayList;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.PathFilter;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.conf.Configuration;

public class MiscUtil
{
    public static void dumpArgs(String[] args)
    {
        for (int i = 0 ; i < args.length ; ++i) {
            System.out.printf("[%d] (%d):%s\n", i, args[i].length(), args[i]);
        }
    }

    public static  String flattenTuple(String str)
    {
        StringBuilder sb = new StringBuilder();
        char sep_tab = '\t';
        for (int ptr = 0 ; ptr < str.length() ; ++ptr) {
            char c = str.charAt(ptr);
            switch (c)
            {
                case ',' :
                    sb.append(sep_tab);
                    break;
                case '[' :
                    break;
                case ']' :
                    break;
                default :
                    sb.append(c);
                    break;
            }  /* end of switch */
        }
        return sb.toString();
    }

    // use hadoop default configuration, usually default hdfs://
    public static boolean pathExist(String path) throws IOException
    {
        Path p = new Path(path);
        Configuration conf = new Configuration();
        FileSystem fs = p.getFileSystem(conf);

        return fs.exists(p);
    }
    public static boolean pathExist(String path, Configuration conf)
        throws IOException
    {
        return pathExist(new Path(path), conf);
    }

    private static PathFilter hiddenFileFilter = new PathFilter(){
        public boolean accept(Path p){
            String name = p.getName();
            return !name.startsWith("_") && !name.startsWith(".");
        }
    };
    public static boolean pathExist(Path pathPattern, Configuration conf)
        throws IOException
    {
        //FileSystem fs = path.getFileSystem(conf);
        //return fs.exists(path);

        FileSystem fs = pathPattern.getFileSystem(conf);
        FileStatus[] matches = fs.globStatus(pathPattern, hiddenFileFilter);
        if (matches == null || matches.length == 0) {
            return false;
        } else {
            return true;
        }
    }

    public static Path[] listFiles(Path pathPattern, Configuration conf)
        throws IOException
    {
        //PathFilter hiddenFileFilter = new PathFilter(){
            //public boolean accept(Path p){
                //String name = p.getName();
                //return !name.startsWith("_") && !name.startsWith(".");
            //}
        //};

        FileSystem fs = pathPattern.getFileSystem(conf);
        FileStatus[] matches = fs.globStatus(pathPattern, hiddenFileFilter);
        if (matches.length == 0) {
            return new Path[0];
        }
        ArrayList<Path> mpaths = new ArrayList<Path>();
        for (FileStatus globStat: matches) {
            if (globStat.isDir()) {
                for(FileStatus stat: fs.listStatus(globStat.getPath(), hiddenFileFilter)) {
                    mpaths.add(stat.getPath());
                }
            } else {
                mpaths.add(globStat.getPath());
            }
        }

        return mpaths.toArray(new Path[mpaths.size()]);
    }

    public static void printArray(String[] objs)
    {
        System.out.printf("String size: %d\n", objs.length);
        for (int i = 0 ; i < objs.length ; ++i) {
            System.out.println(objs[i]);
        }
    }
}
