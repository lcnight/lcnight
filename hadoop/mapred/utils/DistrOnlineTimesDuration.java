import java.util.*;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;

//import org.apache.commons.logging.LogFactory;
//import org.apache.commons.logging.Log;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.util.*;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapred.*;
import org.apache.hadoop.mapred.lib.*;
import org.apache.hadoop.mapred.join.*;
import org.apache.hadoop.mapreduce.Counter;

public class DistrOnlineTimesDuration extends Configured implements Tool {
    public static class AccmMap extends MapReduceBase 
            implements Mapper<Text, Text, Text, Text>
    {
        public void map(Text key, Text value, 
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException  
        {
            output.collect(key, value);
        }
    } 

    public static class AccmReduce extends MapReduceBase
        implements Reducer<Text, Text, Text, LongWritable> 
    {
        private JobConf jobConf = null;
        private Text outKey = new Text();
        private LongWritable one = new LongWritable(1);
        private TreeSet<Integer> durRange = new TreeSet<Integer>();
        
        public void configure(JobConf job) {
            this.jobConf = job;
            String rangesStr = job.get("distr.online.timesduration.range", "0,600,3600");
            System.out.println(rangesStr);
            String [] points = rangesStr.split(",");
            durRange.add(0);
            try { 
                for (int i=0 ; i < points.length ; ++i) {
                    int tmp = Integer.valueOf(points[i]).intValue();
                    durRange.add(tmp);
                }
            } catch (Exception e) {
                e.printStackTrace();
            } 
        }

        public void reduce(Text key, Iterator<Text> values, 
                OutputCollector<Text,LongWritable> output, Reporter reporter) throws IOException 
        {
            int sumdays = 0;
            while (values.hasNext()) {
                String line = values.next().toString();
                String[] cols = line.split("\t", -1);
                sumdays ++;
                int avgDuration = Integer.valueOf(cols[3]).intValue();
                Integer tmp = durRange.lower(avgDuration); 
                int Lower = tmp == null ? 0 : tmp.intValue();
                //if (tmp == null) {
                    //System.out.printf("%s %d", key.toString(), avgDuration);
                //}
                outKey.set(String.format("duration,%d", Lower));
                output.collect(outKey, one);
            }
            outKey.set(String.format("daytimes,%d", sumdays));
            output.collect(outKey, one);
        }
    }

    public static class ResultMap extends MapReduceBase 
            implements Mapper<Text, Text, Text, LongWritable>
    {
        private LongWritable result = new LongWritable(1);
        public void map(Text key, Text value, 
                OutputCollector<Text,LongWritable> output, Reporter reporter) throws IOException  
        {
            try { 
                result.set(Integer.valueOf(value.toString()).longValue());
            } catch (Exception e) {
                e.printStackTrace();
            } finally {
                output.collect(key, result);
            }
        }
    } 

    public static class ResultReduce extends MapReduceBase
        implements Reducer<Text, LongWritable, Text, LongWritable> 
    {
        private LongWritable result = new LongWritable(1);
        public void reduce(Text key, Iterator<LongWritable> values, 
                OutputCollector<Text,LongWritable> output, Reporter reporter) throws IOException 
        {

            long sum = 0;
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
                    "support property: distr.online.timesduration.range=0,600,3600\n\n", 
                    clsName);
            System.exit(-1);
        }

        Configuration conf = getConf();
        JobConf job = new JobConf(conf, getClass());
        String jarName = job.get("user.jar.name", "UserStat.jar");
        job.setJar(jarName);
        Path tempDir = new Path("onlineDistr-temp-"+
                Integer.toString(new Random().nextInt(Integer.MAX_VALUE)));
        job.setJobName(String.format("%s-Accum/%s", clsName, tempDir.getName()));

        // set input format
        job.setInputFormat(KeyValueTextInputFormat.class);
        for (int i = 0; i < args.length - 1 ; ++i) {
            FileInputFormat.addInputPaths(job, args[i]);
        }
        job.setMapperClass(AccmMap.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(Text.class);

        job.setReducerClass(AccmReduce.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(LongWritable.class);

        // set output format
        job.setOutputFormat(TextOutputFormat.class);
        FileOutputFormat.setOutputPath(job, tempDir);

        //job.setNumMapTasks(1);
        int reduceNum = job.getInt("mapred.reduce.tasks", 0);
        System.out.printf("mapred.reduce.tasks = %d\n", reduceNum);
        job.setNumReduceTasks(reduceNum);

        JobClient.runJob(job);


        // ----------------second job-------------------
        JobConf resultConf = new JobConf(conf, getClass()); 
        resultConf.setJar(jarName);
        Path outPath = new Path(args[args.length - 1]);
        job.setJobName(String.format("%s-Result/%s", clsName, outPath.getName()));

        FileInputFormat.addInputPath(resultConf, tempDir);
        resultConf.setInputFormat(KeyValueTextInputFormat.class);
        resultConf.setMapperClass(ResultMap.class);
        resultConf.setMapOutputKeyClass(Text.class);
        resultConf.setMapOutputValueClass(LongWritable.class);

        resultConf.setCombinerClass(ResultReduce.class);

        resultConf.setReducerClass(ResultReduce.class);
        resultConf.setOutputKeyClass(Text.class);
        resultConf.setOutputValueClass(LongWritable.class);
        resultConf.setOutputFormat(TextOutputFormat.class);
        FileOutputFormat.setOutputPath(resultConf, outPath);

        //job.setNumMapTasks(1);
        reduceNum = resultConf.getInt("mapred.reduce.tasks", 1);
        System.out.printf("mapred.reduce.tasks = %d\n", reduceNum);
        resultConf.setNumReduceTasks(reduceNum);
        JobClient.runJob(resultConf);

        FileSystem.get(resultConf).delete(tempDir, true);
        return 0;
    }

    public static void main(String args[]) throws Exception
    {
        int ret = ToolRunner.run(new Configuration(), new DistrOnlineTimesDuration(), args);
        System.exit(ret);
    }
}
