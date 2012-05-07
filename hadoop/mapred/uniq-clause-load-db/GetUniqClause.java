import java.io.IOException;
import java.io.DataInput;
import java.io.DataOutput;
import java.util.*;
        
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.util.*;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.io.*;
import org.apache.hadoop.mapreduce.*;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.input.TextInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;
import org.apache.hadoop.mapreduce.lib.output.TextOutputFormat;
        
public class GetUniqClause extends Configured implements Tool {
        
    public static class TextPair implements WritableComparable<TextPair> {
        private Text first;
        private Text second;

        public TextPair() {
            set(new Text(), new Text());
        }
        public TextPair(Text f, Text s) {
            set(f, s);
        }
        public void set(Text f, Text s) {
            this.first = f;
            this.second = s;
        }
        public void set(String f, String s) {
            this.first = new Text(f);
            this.second = new Text(s);
        }

        public Text getFirst() {
            return first;
        }
        public Text getSecond() {
            return second;
        }

        @Override
        public void write(DataOutput out) throws IOException {
            first.write(out);
            second.write(out);
        }

        @Override
        public void readFields(DataInput in) throws IOException {
            first.readFields(in);
            second.readFields(in);
        }

        @Override
        public int hashCode() {
            return first.hashCode() * 163 + second.hashCode();
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof TextPair) {
                TextPair tp = (TextPair) o;
                return first.equals(tp.first) && second.equals(tp.second);
            }
            return false;
        }

        @Override
        public String toString() {
            return first + "\t" + second;
        }

        @Override
        public int compareTo(TextPair tp) {
            int ret = first.compareTo(tp.first);
            if (ret != 0) {
                return ret;
            }
            return second.compareTo(tp.second);
        }

        static {  // register comparator
            WritableComparator.define(TextPair.class, new TextPairComparator());
        }
    }

    public static class TextPairComparator extends WritableComparator
    {
        private static final Text.Comparator CMP = new Text.Comparator();
        public TextPairComparator() {
            super(TextPair.class);
        }

        @Override
        public int compare(byte[] b1, int s1, int l1, byte[] b2, int s2, int l2) {
            try { 
                int fl1 = WritableUtils.decodeVIntSize(b1[s1]) + readVInt(b1, s1);
                int fl2 = WritableUtils.decodeVIntSize(b2[s2]) + readVInt(b2, s2);
                int cmp = CMP.compare(b1, s1, fl1, b2, s2, fl2);
                if (cmp != 0) {
                    return cmp;
                }

                return CMP.compare(b1, s1 + fl1, l1 - fl1, b2, s2 + fl2, l2 - fl2);

            } catch (IOException e) {
                System.err.println("compare error for TextPair");
                throw new IllegalArgumentException(e);
            }
        }

        @Override
        public int compare(WritableComparable o1, WritableComparable o2) {
            return o1.compareTo(o2);
        }
    }

    //public static class MyGroupingComparator implements RawComparator<TextPair> {
        //@Override
        //public int compare(byte[] b1, int s1, int l1, byte[] b2, int s2, int l2) {
            //try { 
            //} catch (IOException e) {
            //}
        //}

        //@Override
        //public int compare(TextPair o1, TextPair o2) {
            //return o1.compareTo(o2);
        //}
    //}

    public static class TextPairPartitioner extends Partitioner<TextPair,IntWritable>
    {
        @Override
        public int getPartition(TextPair key, IntWritable value, int numPartitions) {
            return Math.abs(key.getFirst().hashCode()) % numPartitions;
        }
    }

 public static class Map extends Mapper<LongWritable, Text, TextPair, IntWritable> 
 {
    private final static IntWritable one = new IntWritable(1);
    private TextPair word = new TextPair();
        
    public void map(LongWritable key, Text value, Context context) throws IOException, InterruptedException {
        String line = value.toString();
        int beg = line.indexOf(']');
        line = line.substring(beg + 1);
        String[] clause = line.split(":");

        if (clause.length < 15 || clause.length > 17 ) {
            Counter malcnt = context.getCounter("Error", "error_format");
            malcnt.increment(1);
            System.err.printf("splitted %d not in [15, 17] %s\n", clause.length, line);
            return;
        }

        word.set("game", clause[0]); // game name
        context.write(word, one); 
        word.set("version", clause[1]); // game version
        context.write(word, one); 
        word.set("gtok", String.format("%s(%s)",clause[0], clause[1])); // gtok
        context.write(word, one); 
        word.set("channel", clause[2]); // channel
        context.write(word, one); 
        word.set("device", clause[4]); // device type
        context.write(word, one); 
        word.set("OS", clause[5]); // OS type
        context.write(word, one); 
        word.set("firmversion", clause[6]); // firmware version
        context.write(word, one); 
        word.set("dtok", String.format("%s(%s)",clause[4], clause[6])); // dtok
        context.write(word, one); 
        word.set("resolution", clause[7]); // resolution
        context.write(word, one); 
        word.set("carrier", clause[8]); // carrier
        context.write(word, one); 
        word.set("lang", clause[10]); // language
        context.write(word, one); 
        word.set("region", clause[11]); // region
        context.write(word, one); 
        word.set("event", clause[14]); // event type
        context.write(word, one); 
    }
 } 

 public static class Reduce extends Reducer<TextPair, IntWritable, TextPair, IntWritable> 
 {
    public void reduce(TextPair key, Iterable<IntWritable> values, Context context) 
      throws IOException, InterruptedException {
        int sum = 0;
        for (IntWritable val : values) {
            sum += val.get();
        }
        context.write(key, new IntWritable(sum));
    }
 }
        
 public int run(String[] args) throws Exception {
    Job job = new Job(getConf(), "Wireless-UniqClause");
    job.setJarByClass(GetUniqClause.class);
    //job.setNumMapTasks(1);
    //job.setNumReduceTasks(1);

    job.setMapperClass(Map.class);
    job.setCombinerClass(Reduce.class);
    job.setReducerClass(Reduce.class);

    // set Partion and Group
    job.setPartitionerClass(TextPairPartitioner.class);
    job.setGroupingComparatorClass(TextPairComparator.class);

    // the map output is TextPair, IntWritable
    job.setMapOutputKeyClass(TextPair.class);
    job.setMapOutputValueClass(IntWritable.class);

    // the reduce output is TextPair, IntWritable
    job.setOutputKeyClass(TextPair.class);
    job.setOutputValueClass(IntWritable.class);
        
    job.setInputFormatClass(TextInputFormat.class);
    job.setOutputFormatClass(TextOutputFormat.class);
    //TextInputFormat.addInputPath(job, new Path(args[0]));
    TextInputFormat.addInputPaths(job, args[0]);
    TextOutputFormat.setOutputPath(job, new Path(args[1]));
        
    job.waitForCompletion(true);
    return 0;
 }

 public static void main(String args[]) throws Exception
 {
     if (args.length < 3) {
         System.err.println("Usage: GetUniqClause <input-dir> <out-dir> -<m|d|b> <db-options>\n" + 
                 "\tinput-dir   for mutli paths, separated by comma(,)\n" +
                 "\t-m          only do map reduce operation\n" + 
                 "\t-d          only do database operation\n" +
                 "\t-b          do both mapred and database operation\n\n" + 
                 "\tdb options example:\n\tuser:password@ip[:port]/database"
                 );
         System.exit(-1);
     }

     boolean do_mapred = false;
     boolean do_db = false;
     if (args[2].equals("-m")) { do_mapred = true; } 
     if (args[2].equals("-d")) { do_db = true; } 
     if (args[2].equals("-b")) {
         do_mapred = true;
         do_db = true;
     } 

     int ret = 0;
     if (do_mapred) {
         ToolRunner.run(new Configuration(), new GetUniqClause(), args);
         System.err.printf("\nmapred end with code: %d\n\n", ret);
         if (ret != 0) {
             System.exit(ret);
         }
     }

     if (do_db) {
         if (args.length != 4) {
             System.err.println("db-option maybe not provided\n");
             System.exit(2);
         }
         System.out.println("loading unique phrase into database table\n");
         ret = buildEnv.setup_env(args[1], args[3]);
         System.err.printf("\ndatabase load end with code: %d\n\n", ret);
         System.exit(ret);
     }
 }
}
