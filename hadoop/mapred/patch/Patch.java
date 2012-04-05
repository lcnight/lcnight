import java.util.Date;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import org.apache.hadoop.io.*;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.fs.Path;
//import org.apache.hadoop.mapred.*;
import org.apache.hadoop.mapreduce.*;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;
        
public class Patch {
        
 public static class Map extends Mapper<LongWritable, Text, Text, IntWritable> {
    SimpleDateFormat ymd = new SimpleDateFormat("yyyyMMdd");
    SimpleDateFormat ff = new SimpleDateFormat("dd/MMM/yyyy:HH:mm:ss Z");
    String patching = "patching";
    String patchcomplete = "patchComplete";
        
    private Text token = new Text();
    private final static IntWritable one = new IntWritable(1);

    public void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
        String line = value.toString();

        int beg = line.indexOf('[') + 1;
        int end = line.indexOf(']');
        String time_str = line.substring(beg, end);
        String dt_tok = "19700101";
        try { 
            Date dt = ff.parse(time_str);
            dt_tok = ymd.format(dt);
        } catch (ParseException e) {

        }
        
        end = line.indexOf(' ');
        String ip = line.substring(0, end);
            
        if (line.indexOf("New/patching?message=patching") > 0) {
            String tok = String.format("%s_%s_%s", dt_tok, ip, patching);
            token.set(tok);
            context.write(token, one);
        } 
        else if (line.indexOf("New/patchComplete/?message=patchComplete") > 0) {
            String tok = String.format("%s_%s_%s", dt_tok, ip, patchcomplete);
            token.set(tok);
            context.write(token, one);
        } else {
            //nothing
        }

    }
 } 
        
 public static class Reduce extends Reducer<Text, IntWritable, Text, IntWritable> {

    public void reduce(Text key, Iterable<IntWritable> values, Context context) 
      throws IOException, InterruptedException {
        int sum = 0;
        for (IntWritable val : values) {
            sum += val.get();
        }
        context.write(key, new IntWritable(sum));
    }
 }
        
 public static void main(String[] args) throws Exception {
     Configuration conf = new Configuration();
     Job job = new Job(conf, "client_patch_ip");
     job.setJarByClass(Patch.class);

     job.setNumReduceTasks(1);

     job.setMapperClass(Map.class);
     //job.setCombinerClass(Reduce.class);
     job.setReducerClass(Reduce.class);

     job.setOutputKeyClass(Text.class);
     job.setOutputValueClass(IntWritable.class);

     job.setInputFormatClass(TextInputFormat.class);
     job.setOutputFormatClass(TextOutputFormat.class);

     FileInputFormat.addInputPath(job, new Path(args[0]));
     FileOutputFormat.setOutputPath(job, new Path(args[1]));

     job.waitForCompletion(true);
     //JobClient.runJob(job);
 }
        
}
