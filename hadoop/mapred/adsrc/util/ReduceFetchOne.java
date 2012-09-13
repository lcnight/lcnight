package util;

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

/**
 * @brief  fetch only one value for each key, ignore others
 */
public class ReduceFetchOne extends MapReduceBase
    implements Reducer<Text, Text, Text, Text>
{
    public void reduce(Text key, Iterator<Text> values,
            OutputCollector<Text,Text> output, Reporter reporter) throws IOException {

        output.collect(key, values.next());
        return;

        //while (values.hasNext()) {
        //}
    }
}

