import java.util.*;
import java.io.IOException;
import java.io.DataInput;
import java.io.DataOutput;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.util.*;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapred.*;
import org.apache.hadoop.mapred.lib.*;
import org.apache.hadoop.mapred.join.*;
import org.apache.hadoop.mapreduce.Counter;
//import org.apache.commons.logging.LogFactory;
//import org.apache.commons.logging.Log;

public class MapMultiJoinDriver extends Configured implements Tool {

    public static class Map extends MapReduceBase
            implements Mapper<Text, TupleWritable, Text, Text>
    {
        private Text joinCols = new Text();
        private JobConf jconf = null;
        private String joinType = "inner";
        private int joinTblNum = 2;
        public void configure(JobConf conf)
        {
            this.jconf = conf;
            joinType = conf.get("mapred.map.join.type");
            joinTblNum = conf.getInt("mapred.map.join.tblNum", 2);
            //System.out.printf("mapred.map.join.type = %s, tblNum = %d\n", joinType, joinTblNum);
        }

        public void map(Text key, TupleWritable value,
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException
        {
            if (joinType.equals("inner")) {
                for (int i = 0 ; i < joinTblNum ; ++i) {
                    if (!value.has(i)) {
                        return;
                    }
                }
            } 
            else if (joinType.equals("left")) {
                if (!value.has(0)) {
                    return;
                }
            } 
            else if (joinType.equals("right")) {
                if (!value.has(joinTblNum - 1)) {
                    return;
                }
            } else {
                // outer join
            }

            String flatStr = MiscUtil.flattenTuple(value.toString());
            joinCols.set(flatStr);
            output.collect(key, joinCols);
        }
    }

    //public static class Reduce extends MapReduceBase
            //implements Reducer<Text, Text, Text, Text>
    //{
        //public void reduce(Text key, Iterator<Text> values,
                //OutputCollector<Text,Text> output, Reporter reporter) throws IOException {
            //while (values.hasNext()) {
            //}
        //}
    //}

    private void showUsage(String clsName)
    {
        System.out.printf("Usage: %s <left|right|inner|outer> input... output\n" +
                "\tjoin and produce items in MapTask\n\n", clsName, clsName);
        System.exit(0);
    }

    public int run(String[] args) throws Exception {
        // final Log LOG = LogFactory.getLog("main-test");
        String clsName = this.getClass().getName();
        if (args.length < 4 || 
                !( args[0].equals("left") || args[0].equals("right") ||
                    args[0].equals("inner") || args[0].equals("outer") )) {
            showUsage(clsName);
        }

        Configuration conf = getConf();
        JobConf job = new JobConf(conf, getClass());
        String jarName = job.get("user.jar.name", "UserStat.jar");
        job.setJar(jarName);
        job.set("mapred.map.join.type", args[0]);
        int tblNum = args.length - 2;
        job.setInt("mapred.map.join.tblNum", tblNum);

        Path[] p = new Path[tblNum];
        for (int i = 1 ; i < args.length - 1 ; ++i) {
            if (!MiscUtil.pathExist(args[i], conf)) {
                System.err.printf("not exists PATH: %s\n", args[i]);
                System.exit(0);
            }
            FileInputFormat.addInputPaths(job, args[i]);
            p[i - 1] = new Path(args[i]);
        }

        StringBuilder sb = new StringBuilder();
        sb.append("{ ");
        for (int i = 0 ; i < tblNum ; ++i) {
           sb.append(p[i].getName());
           sb.append(" | ");
        }
        sb.append("} ");
        job.setJobName(String.format("%s %s\n", clsName, sb.toString()));

        job.set("mapred.join.expr", CompositeInputFormat.compose("outer",
                    KeyValueTextInputFormat.class, FileInputFormat.getInputPaths(job)));
        System.out.printf("mapred.join.expr = %s\n", job.get("mapred.join.expr"));

        job.setInputFormat(CompositeInputFormat.class);
        job.setMapperClass(Map.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(Text.class);

        FileOutputFormat.setOutputPath(job, new Path(args[args.length - 1]));

        job.setOutputFormat(TextOutputFormat.class);
        job.setReducerClass(IdentityReducer.class);
        //job.setOutputKeyClass(Text.class);
        //job.setOutputValueClass(Text.class);

        // set Partion and Group
        //job.setPartitionerClass(TextPartitioner.class);
        //job.setGroupingComparatorClass(TextComparator.class);
        //job.setNumMapTasks(1);
        int reduceNum = job.getInt("mapred.reduce.tasks", 0);
        System.out.printf("mapred.reduce.tasks = %d\n", reduceNum);
        job.setNumReduceTasks(reduceNum);

        JobClient.runJob(job);

        return 0;
    }

    public static void main(String args[]) throws Exception
    {
        int ret = ToolRunner.run(new Configuration(), new MapMultiJoinDriver(), args);
        System.exit(ret);
    }
}
