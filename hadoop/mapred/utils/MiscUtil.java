import java.io.IOException;
import org.apache.hadoop.fs.Path;
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
    public static boolean pathExist(Path path, Configuration conf)
        throws IOException
    {
        FileSystem fs = path.getFileSystem(conf);
        return fs.exists(path);
    }

    public static void printArray(String[] objs)
    {
        System.out.printf("String size: %d\n", objs.length);
        for (int i = 0 ; i < objs.length ; ++i) {
            System.out.println(objs[i]);
        }

    }
}
