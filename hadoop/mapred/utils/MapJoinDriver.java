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

public class MapJoinDriver extends Configured implements Tool {

    public static class Map extends MapReduceBase
            implements Mapper<Text, TupleWritable, Text, Text>
    {
        private Text joinCols = new Text();
        private JobConf jconf = null;
        private String joinType = "inner";
        public void configure(JobConf conf)
        {
            this.jconf = conf;
            joinType = conf.get("mapred.map.join.type");

            //System.out.printf("mapred.map.join.type = %s\n", joinType);
        }

        public void map(Text key, TupleWritable value,
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException
        {
            if (joinType.equals("inner")) {
                if (!(value.has(0) && value.has(1))) {
                    return;
                }
            } 
            else if (joinType.equals("left")) {
                if (!value.has(0)) {
                    return;
                }
            } 
            else if (joinType.equals("right")) {
                if (!value.has(1)) {
                    return;
                }
            } 
            else if (joinType.equals("leftonly")) {
                if (!(value.has(0) && !value.has(1))) {
                    return;
                }
            } 
            else if (joinType.equals("rightonly")) {
                if (!(!value.has(0) && value.has(1))) {
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

    // reduce in map
    //public static class Reduce extends MapReduceBase
            //implements Reducer<Text, Text, Text, Text>
    //{
        //Text words = new Text();
        //public void reduce(Text key, Iterator<Text> values,
                //OutputCollector<Text,Text> output, Reporter reporter) throws IOException {
            //while (values.hasNext()) {
                //String flatStr = MiscUtil.flattenTuple(values.next().toString());
                //String[] items = flatStr.split("\t", -1);
                //if (items[0].length() == 0) {
                    //continue;
                //} else {
                    //words.set(flatStr);
                    //output.collect(key, words);
                //}
            //}
        //}
    //}

    private void showUsage(String clsName)
    {
        System.out.printf("Usage: %s <subtract|left|leftonly|right|rightonly|inner|outer> inputA inputB output\n" +
                "\tjoin and produce items in MapTask\n\n", clsName, clsName);
        System.exit(0);
    }

    public int run(String[] args) throws Exception {
        // final Log LOG = LogFactory.getLog("main-test");
        String clsName = this.getClass().getName();
        if (args.length != 4 || 
                !(args[0].equals("subtract") || 
                    args[0].equals("left") || args[0].equals("leftonly") || 
                    args[0].equals("right") || args[0].equals("rightonly") || 
                    args[0].equals("inner") || args[0].equals("outer") )) {
            showUsage(clsName);
        }

        Configuration conf = getConf();
        JobConf job = new JobConf(conf, getClass());
        job.setJobName(clsName);
        job.set("mapred.map.join.type", args[0]);

        FileOutputFormat.setOutputPath(job, new Path(args[args.length - 1]));

        for (int i = 1 ; i < args.length - 1 ; ++i) {
            if (!MiscUtil.pathExist(args[i], conf)) {
                System.err.printf("not exists PATH: %s\n", args[i]);
                System.exit(0);
            }
            FileInputFormat.addInputPaths(job, args[i]);
        }
        job.set("mapred.join.expr", CompositeInputFormat.compose("outer",
                    KeyValueTextInputFormat.class, FileInputFormat.getInputPaths(job)));

        System.out.printf("mapred.join.expr = %s\n", job.get("mapred.join.expr"));

        job.setInputFormat(CompositeInputFormat.class);
        // job.setMapperClass(IdentityMapper.class);
        job.setMapperClass(Map.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(Text.class);

        job.setOutputFormat(TextOutputFormat.class);
        //job.setReducerClass(Reduce.class);
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
        int ret = ToolRunner.run(new Configuration(), new MapJoinDriver(), args);
        System.exit(ret);
    }
}
