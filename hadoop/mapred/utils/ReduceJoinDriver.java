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
import org.apache.hadoop.io.WritableComparator;


public class ReduceJoinDriver extends Configured implements Tool {
    
    public static class FirstMap extends MapReduceBase 
            implements Mapper<Text, Text, Text, Text>
    {
        private byte[] firstTok = null;
        private JobConf jobConf = null;
        public void configure(JobConf job) {
            this.jobConf = job;
            firstTok = "@0".getBytes();
        }

        public void map(Text key, Text value, 
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException  {
            key.append(firstTok, 0, 2);
            value.append(firstTok, 0, 2);
            output.collect(key, value);
        }
    } 
    public static class SecondMap extends MapReduceBase 
            implements Mapper<Text, Text, Text, Text>
    {
        private byte[] secondTok = null;
        private JobConf jobConf = null;
        public void configure(JobConf job) {
            this.jobConf = job;
            secondTok = "@1".getBytes();
        }

        public void map(Text key, Text value, 
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException 
        {
            key.append(secondTok, 0, 2);
            value.append(secondTok, 0, 2);
            output.collect(key, value);
        }
    } 

    public static class Reduce extends MapReduceBase
        implements Reducer<Text, Text, Text, Text> 
    {
        ArrayList<String> firstTbl = new ArrayList<String>();
        ArrayList<String> secondTbl = new ArrayList<String>();
        ArrayList<String> result = new ArrayList<String>();
        private Text joinCols = new Text();
        private JobConf jobConf = null;
        private String joinType = "inner";
        public void configure(JobConf job) {
            this.jobConf = job;
            joinType = job.get("mapred.reduce.join.type");

            //System.out.printf("get mapred.reduce.join.type = %s\n", joinType);
        }

        
        public void reduce(Text key, Iterator<Text> values, 
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException
        {
            firstTbl.clear();
            secondTbl.clear(); 
            result.clear(); 
            while (values.hasNext()) {
                String tmpStr = values.next().toString();
                if (tmpStr.endsWith("@0")) {
                    firstTbl.add(tmpStr.substring(0, tmpStr.length() - 2));
                } 
                else if (tmpStr.endsWith("@1")) {
                    secondTbl.add(tmpStr.substring(0, tmpStr.length() - 2));
                } 
                else {
                    System.err.printf("error format value: %s\n", tmpStr);
                }
            }

            if (joinType.equals("inner")) {
            buildInnerJoin();
            } 
            else if (joinType.equals("outer")) {
            buildInnerJoin();
            buildLeftOnlyJoin();
            buildRightOnlyJoin();
            }
            else if (joinType.equals("left")) {
            buildInnerJoin();
            buildLeftOnlyJoin();
            } 
            else if (joinType.equals("leftonly")) {
            buildLeftOnlyJoin();
            } 
            else if (joinType.equals("right")) {
            buildInnerJoin();
            buildRightOnlyJoin();
            } 
            else if (joinType.equals("rightonly")) {
            buildRightOnlyJoin();
            } 
            else {
                //throw new Exception("unknow reduce join type");
                System.err.printf("unknow reduce join type: %s\n", joinType);
            }

            String tmpKey = key.toString();
            String realKey = tmpKey.substring(0, tmpKey.length() - 2);

            //System.out.printf("reduce key %s \n", realKey);

            key.set(realKey);
            for (int i = 0 ; i < result.size() ; ++i) {
                joinCols.set(result.get(i));
                output.collect(key, joinCols);
            }
        }

        //public void buildInnerJoin(ArrayList<String> left, ArrayList<String> right)
        public void buildInnerJoin()
        {
            for (int i =0 ; i < firstTbl.size() ; ++i) {
                for (int j=0 ; j < secondTbl.size() ; ++j) {
                    result.add(String.format("%s\t%s", firstTbl.get(i), secondTbl.get(j)));
                }
            }
        }
        public void buildLeftOnlyJoin()
        {
            if (firstTbl.size() > 0 && secondTbl.size() == 0) {
                for (int i =0 ; i < firstTbl.size() ; ++i) {
                    result.add(String.format("%s\t", firstTbl.get(i)));
                }
            }
        }
        public void buildRightOnlyJoin()
        {
            if (firstTbl.size() == 0 && secondTbl.size() > 0) {
                for (int j =0 ; j < secondTbl.size() ; ++j) {
                    result.add(String.format("\t%s", secondTbl.get(j)));
                }
            }
        }
    }

    public static class ReduceJoinPartitioner implements Partitioner<Text, Text> 
    {
        @Override
        public void configure(JobConf conf) {}

        @Override
        public int getPartition(Text key, Text value, int numPartitions)
        {
            String keytmp = key.toString();
            String realKey = keytmp.substring(0, keytmp.length() - 2);
            return (realKey.hashCode() & Integer.MAX_VALUE) % numPartitions;
        }
    }

    public static class ReduceJoinGrouping extends WritableComparator
    {
        private int tokLen = 2;
        public ReduceJoinGrouping () 
        {
            super(Text.class);
        }

        public int compare(byte[] b1, int s1, int l1, byte[] b2, int s2, int l2) {
            int n1 = WritableUtils.decodeVIntSize(b1[s1]);
            int n2 = WritableUtils.decodeVIntSize(b2[s2]); 
            //String ss1 = new String(b1, s1+n1, l1-n1-tokLen);
            //String ss2 = new String(b2, s2+n2, l2-n2-tokLen);
            //System.out.printf("cmp %s %s\n", ss1, ss2);
            return compareBytes(b1, s1+n1, l1-n1-tokLen, b2, s2+n2, l2-n2-tokLen);
        }

        public int compare(WritableComparable a, WritableComparable b) 
        {
            if (a instanceof Text && b instanceof Text) {
                byte[] b1 = ((Text)a).getBytes();
                byte[] b2 = ((Text)b).getBytes();
                //System.out.printf("cmpab %s %s\n",  ((Text)a).toString(), ((Text)b).toString());
                return compareBytes(b1, 0, ((Text)a).getLength() - 2, 
                        b2, 0, ((Text)b).getLength() - 2);
            }
            System.out.printf("why not Text");
            return 0;
        }
    }

    public int run(String[] args) throws Exception {
        // final Log LOG = LogFactory.getLog("main-test");

        String clsName = this.getClass().getName();
        if (args.length < 4 || 
                !(args[0].equals("inner") || args[0].equals("outer") || 
                    args[0].equals("left") || args[0].equals("leftonly") || 
                    args[0].equals("rightonly") || args[0].equals("right"))) {
            System.out.printf("Usage: %s <inner|outer|left|leftonly|right|rightonly> inputA inputB output\n\n"+
                    "\treduce join 2 inputs\n\n"
                    , clsName);
            System.exit(-1);
        }

        Configuration conf = getConf();
        JobConf job = new JobConf(conf, getClass());
        job.setJobName(clsName);

        job.set("mapred.reduce.join.type", args[0]);

        for (int i = 1 ; i < args.length - 1 ; ++i) {
            if (!MiscUtil.pathExist(args[i], conf)) {
                System.err.printf("Exit, not exists PATH: %s\n", args[i]);
                System.exit(0);
            }
        }
        // set input format
        MultipleInputs.addInputPath(job, new Path(args[1]), 
                KeyValueTextInputFormat.class, FirstMap.class);
        MultipleInputs.addInputPath(job, new Path(args[2]), 
                KeyValueTextInputFormat.class, SecondMap.class);

        //job.setInputFormat(KeyValueTextInputFormat.class);
        //job.setMapperClass(Map.class);
        //job.setMapOutputKeyClass(Text.class);
        //job.setMapOutputValueClass(Text.class);
        //for (int i = 0 ; i < args.length - 1 ; ++i) {
            //FileInputFormat.addInputPaths(job, args[i]);
        //}

        // set output format
        job.setReducerClass(Reduce.class);
        job.setOutputFormat(TextOutputFormat.class);
        job.setOutputKeyClass(Text.class);
        job.setOutputValueClass(Text.class);
        FileOutputFormat.setOutputPath(job, new Path(args[args.length - 1]));

        // set Partion and Group
        job.setPartitionerClass(ReduceJoinPartitioner.class);
        job.setOutputValueGroupingComparator(ReduceJoinGrouping.class);
        //job.setNumMapTasks(1);
        int reduceNum = job.getInt("mapred.reduce.tasks", 0);
        System.out.printf("mapred.reduce.tasks = %d\n", reduceNum);
        job.setNumReduceTasks(reduceNum);

        JobClient.runJob(job);

        return 0;
    }

    public static void main(String args[]) throws Exception
    {
        int ret = ToolRunner.run(new Configuration(), new ReduceJoinDriver(), args);
        System.exit(ret);
    }
}
