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

public class UniqKeyValue extends Configured implements Tool {

    //public static class Map extends MapReduceBase
            //implements Mapper<Text, Text, Text, Text>
    //{
        //private JobConf jobConf = null;
        //public void configure(JobConf job) {
            //this.jobConf = job;
        //}

        //public void map(Text key, Text value,
                //OutputCollector<Text,Text> output, Reporter reporter) throws IOException  {
            //output.collect(key, NullWritable.get());
        //}
    //}

    public static class Reduce extends MapReduceBase
        implements Reducer<Text, Text, Text, Text>
    {
        //private IntWritable one = new IntWritable(1);
        //private Text uniqvalue = new Text();

        public void reduce(Text key, Iterator<Text> values,
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException {

            output.collect(key, values.next());

            while (values.hasNext()) {
                System.out.printf("another value: %s for key: %s\n",
                        values.next().toString(), key.toString());
            }
        }
    }

    public int run(String[] args) throws Exception {
        // final Log LOG = LogFactory.getLog("main-test");

        String clsName = this.getClass().getName();
        if (args.length < 2) {
            System.out.printf("Usage: %s inputs... output\n\n"+
                    "\tcombine mutil files (key=>value) to one\n\n" , clsName);
            System.exit(-1);
        }

        Configuration conf = getConf();
        JobConf job = new JobConf(conf, getClass());
        job.setJobName(clsName);

        job.setMapperClass(IdentityMapper.class);
        job.setReducerClass(Reduce.class);

        // set input format
        job.setInputFormat(KeyValueTextInputFormat.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(Text.class);
        for (int i = 0 ; i < args.length - 1 ; ++i) {
            if (!MiscUtil.pathExist(args[i], conf)) {
                continue;
            }
            System.out.printf("add path: %s\n", args[i]);
            FileInputFormat.addInputPaths(job, args[i]);
        }

        // set output format
        job.setOutputFormat(TextOutputFormat.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        FileOutputFormat.setOutputPath(job, new Path(args[args.length - 1]));

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
        int ret = ToolRunner.run(new Configuration(), new UniqKeyValue(), args);
        System.exit(ret);
    }
}
