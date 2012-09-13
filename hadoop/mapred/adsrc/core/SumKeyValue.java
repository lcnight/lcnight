package core;

import java.util.*;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

//import org.apache.commons.logging.LogFactory;
//import org.apache.commons.logging.Log;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.util.*;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapred.*;
import org.apache.hadoop.mapred.lib.*;
import org.apache.hadoop.mapred.join.*;
import org.apache.hadoop.mapreduce.Counter;

import util.*;


/**
 * @class SumKeyValue
 * @brief sum value for the same key, raw file line format: <key \t num>
 *          raw line must meet the format requirement, 
 *          or meaningless result, running exception
 */
public class SumKeyValue extends Configured implements Tool {

    public static class Map extends MapReduceBase
            implements Mapper<Text, Text, Text, LongWritable>
    {
        //private Text realKey = new Text();
        private LongWritable num = new LongWritable(0);

        private JobConf jobConf = null;
        public void configure(JobConf job) {
            this.jobConf = job;
        }

        public void map(Text key, Text value,
                OutputCollector<Text,LongWritable> output, Reporter reporter) throws IOException  {
            String sv = value.toString();
            try { 
                num.set(Long.valueOf(sv));
            } catch (Exception ex) {
                ex.printStackTrace();
                System.err.printf("Cannot convert to number: %s\n", sv);
                return;
            } 

            output.collect(key, num);
        }
    }

    //public static class Reduce extends MapReduceBase
        //implements Reducer<Text, LongWritable, Text, LongWritable>
    //{
        //private LongWritable sumresult = new LongWritable(0);

        //private JobConf jobConf = null;
        //public void configure(JobConf job) {
            //this.jobConf = job;
        //}

        //public void reduce(Text key, Iterator<LongWritable> values,
                //OutputCollector<Text,LongWritable> output, Reporter reporter) throws IOException {

            //long sum = 0;
            //while (values.hasNext()) {
                //sum += values.next().get();
            //}
            //sumresult.set(sum);
            //output.collect(key, sumresult);
        //}
    //}

    public int run(String[] args) throws Exception {
        // final Log LOG = LogFactory.getLog("main-test");

        String clsName = this.getClass().getName();
        if (args.length < 2) {
            System.out.printf("Usage: %s inputs... output\n\n"+
                    "\tsum value for the same key (input line format: key\tnum)\n\n" , clsName);
            System.exit(-1);
        }

        Configuration conf = getConf();
        JobConf job = new JobConf(conf, getClass());
        String jarName = job.get("user.jar.name", "Topics.jar");
        job.setJar(jarName);
        Path outPath = new Path(args[args.length - 1]);
        job.setJobName(String.format("%s/%s", clsName, outPath.getName()));

        // set input format
        job.setInputFormat(KeyValueTextInputFormat.class);
        for (int i = 0 ; i < args.length - 1 ; ++i) {
            if (args[i].indexOf('*') > 0) {  // find wildcard, expand it
                Path[] matchs = MiscUtil.listMatch(new Path(args[i]), conf);
                for(Path p : matchs) {
                    System.out.printf("add path: %s\n", p.toString());
                    FileInputFormat.addInputPath(job, p);
                }
            } 
            else if (!MiscUtil.pathExist(args[i], conf)) {
                continue;
            } else {
                System.out.printf("add path: %s\n", args[i]);
                FileInputFormat.addInputPaths(job, args[i]);
            }
        }
        job.setMapperClass(Map.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(LongWritable.class);

        job.setCombinerClass(LongSumReducer.class);

        job.setReducerClass(LongSumReducer.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(LongWritable.class);

        // set output format
        job.setOutputFormat(TextOutputFormat.class);
        FileOutputFormat.setOutputPath(job, outPath);

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
        int ret = ToolRunner.run(new Configuration(), new SumKeyValue(), args);
        System.exit(ret);
    }
}
