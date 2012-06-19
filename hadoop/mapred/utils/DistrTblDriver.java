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

public class DistrTblDriver extends Configured implements Tool {

    public static class Map extends MapReduceBase 
            implements Mapper<Text, Text, Text, IntWritable>
    {
        private Text outKey = new Text();
        private IntWritable one = new IntWritable(1);

        enum DistrTblCounter { ErrorValueFormat };
        private int valueColsNum = 6;
        private JobConf jobConf = null;
        private TblColsParser parser = new TblColsParser();
        public void configure(JobConf job) {
            this.jobConf = job;
            valueColsNum = job.getInt("distr.value.colsNum", 6);
            parser.setSkip(job.getInt("distr.skip.index", 2));
            parser.setRegularIndex(job.getInt("distr.regular.index", 4));
            parser.setNullDefault("0");
        }

        public void map(Text key, Text value, 
                OutputCollector<Text,IntWritable> output, Reporter reporter) throws IOException  
        {
            String line = value.toString();

            String [] linecols = line.split("\t",-1);
            if (linecols.length != valueColsNum) {
                reporter.incrCounter(DistrTblCounter.ErrorValueFormat, 1);
                return;
            }

            parser.setContent(line);
            String[] groups = parser.getGroupArray();
            for (int i = 0 ; i < groups.length ; ++i) {
                //String [] cols = groups[i].split(",",-1);
                //if (cols.length < 4) {
                    //System.err.printf("key: %s value: %s  groups %s < 4\n", key.toString(), line, groups[i]);
                //}
                outKey.set(groups[i]);
                output.collect(outKey, one);
            }
        }
    } 

    public static class Reduce extends MapReduceBase
        implements Reducer<Text, IntWritable, Text, IntWritable> 
    {
        IntWritable result = new IntWritable();
        public void reduce(Text key, Iterator<IntWritable> values, 
                OutputCollector<Text,IntWritable> output, Reporter reporter) throws IOException 
        {
            int sum = 0;
            while (values.hasNext()) {
                sum += values.next().get();
            }
            result.set(sum);
            output.collect(key, result);
        }
    }

    public int run(String[] args) throws Exception {
        // final Log LOG = LogFactory.getLog("main-test");

        String clsName = this.getClass().getName();
        if (args.length < 2) {
            System.out.printf("Usage: %s input output\n\n\t" + 
                    "support property: distr.skip.index=idxNum\t# default index 2\n\n", 
                    clsName);
            System.exit(-1);
        }

        JobConf job = new JobConf(getConf(), getClass());
        String jarName = job.get("user.jar.name", "UserStat.jar");
        job.setJar(jarName);
        Path outPath = new Path(args[args.length - 1]);
        job.setJobName(String.format("%s/%s", clsName, outPath.getName()));

        job.setMapperClass(Map.class);
        job.setReducerClass(Reduce.class);

        // set input format
        job.setInputFormat(KeyValueTextInputFormat.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(IntWritable.class);
        for (int i = 0; i < args.length - 1 ; ++i) {
            FileInputFormat.addInputPaths(job, args[i]);
        }

        // set output format
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(IntWritable.class);
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
        int ret = ToolRunner.run(new Configuration(), new DistrTblDriver(), args);
        System.exit(ret);
    }
}
