package core;

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

import util.*;

public class JoinDriver extends Configured implements Tool {

    public static class Map extends MapReduceBase
            implements Mapper<Text, TupleWritable, Text, Text>
    {
        private Text word = new Text();
        //private String sep = "\t";

        public void map(Text key, TupleWritable value,
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException
        {
            //StringBuilder sb = new StringBuilder();
            //Iterator<Writable> it = value.iterator();
            //while (it.hasNext()) {
                //Object tmp = it.next();
                //if (tmp instanceof TupleWritable) {
                    ////TupleWritable obj = (TupleWritable)tmp;
                    //Iterator<Writable> innerit = ((TupleWritable)tmp).iterator();
                    //while (innerit.hasNext()) {
                        //sb.append(innerit.next().toString());
                        //sb.append(sep);
                    //}
                //} else {
                    //sb.append(tmp.toString());
                    //sb.append(sep);
                //}
            //}
            //sb.deleteCharAt(sb.length() - 1);
            //word.set(sb.toString());

            word.set(value.toString());
            output.collect(key, word);
        }
    }

    public static class Reduce extends MapReduceBase
            implements Reducer<Text, Text, Text, Text>
    {
        Text words = new Text();

        public void reduce(Text key, Iterator<Text> values,
                OutputCollector<Text,Text> output, Reporter reporter) throws IOException 
        {
            while (values.hasNext()) {
                words.set(MiscUtil.flattenTuple(values.next().toString()));
                output.collect(key, words);
            }
        }
    }

    private void showUsage(String clsName)
    {
        System.out.printf("Usage: %s <inner|outer> inputs... output\n" +
                "Usage: %s -expr <join-expression> output\n\n" +
                "\teither join multi files, or join files according to specified expression\n\n", clsName, clsName);
        System.exit(-1);
    }

    public int run(String[] args) throws Exception {
        // final Log LOG = LogFactory.getLog("main-test");
        String clsName = this.getClass().getName();
        if (args.length < 3) {
            showUsage(clsName);
        }

        JobConf job = new JobConf(getConf(), getClass());
        job.setJobName(clsName);
        String jarName = job.get("user.jar.name", "Topics.jar");
        job.setJar(jarName);

        if (args[0].equals("-expr")) {
            System.out.printf("set mapred.join.expr = %s\n", args[1]);
            job.set("mapred.join.expr", args[1]);
        } else {
            if (!(args[0].equals("inner") || args[0].equals("outer"))) {
                showUsage(clsName);
            }
            for (int i = 1 ; i<args.length - 1 ; ++i) {
                FileInputFormat.addInputPaths(job, args[i]);
            }

            job.set("mapred.join.expr", CompositeInputFormat.compose(args[0],
                        KeyValueTextInputFormat.class, FileInputFormat.getInputPaths(job)));
            for(Path p : FileInputFormat.getInputPaths(job))  {
                System.out.println(p.toString());
            }
        }
        FileOutputFormat.setOutputPath(job, new Path(args[args.length - 1]));

        System.out.printf("mapred.join.expr = %s\n", job.get("mapred.join.expr"));

        job.setInputFormat(CompositeInputFormat.class);

        // job.setMapperClass(IdentityMapper.class);
        job.setMapperClass(Map.class);
        job.setMapOutputKeyClass(Text.class);
        job.setMapOutputValueClass(Text.class);

        job.setOutputFormat(TextOutputFormat.class);
        job.setReducerClass(Reduce.class);
        //job.setReducerClass(IdentityReducer.class);
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
        int ret = ToolRunner.run(new Configuration(), new JoinDriver(), args);
        System.exit(ret);
    }
}
