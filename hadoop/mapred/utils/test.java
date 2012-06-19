import java.io.*;
import java.util.*;
import org.apache.hadoop.conf.*;
import org.apache.hadoop.util.*;
import org.apache.hadoop.fs.*;
//import org.apache.commons.logging.LogFactory;
//import org.apache.commons.logging.Log;

public class test 
{
    public static void ipdist_test(String[] args)  throws Exception
    {
        String dburi = "mysql://root:ta0mee@10.1.1.60/city_ip_db?" +
            "autoReconnect=true&useUnicode=true&characterEncoding=utf8";
        IPDistr ipd = new IPDistr(dburi);

        long ip = 2771424027L;
        for (int i=0 ; i < args.length ; ++i) {
            ip = Long.valueOf(args[i]);
            System.out.printf("ip: %d,\nProvince Code: %d, Province Name: %s,\ncity code: %d, city name: %s\n",
                    ip, ipd.getIPProvinceCode(ip), ipd.getIPProvinceName(ip), 
                    ipd.getIPCityCode(ip), ipd.getIPCityName(ip));
        }
        
    }

    public static void date_test(String[] args)  throws Exception
    {
        for (int i = 0 ; i < args.length ; ++i) {
            int date = Integer.valueOf(args[i]);

            int length = args[i].length();
            if (length == 6) {
                System.out.printf("Date: %d, +3: %d, -3: %d\n", 
                        date, DateUtil.getMonthOff(date, 3), DateUtil.getMonthOff(date, -3));
            }
            else if (length == 8) {
                System.out.printf("Date: %d, +20: %d, -20: %d\n", 
                        date, DateUtil.getDayOff(date, 20), DateUtil.getDayOff(date, -20));
            } else {
                System.out.printf("invalid: %s", args[i]);
            }
        }
        
    }

    public static class TestTools extends Configured implements Tool 
    {
        public int run(String[] args) throws Exception {
            Configuration conf = getConf();

            //for (int i = 0 ; i < args.length ; ++i) {
                ////System.out.println(MiscUtil.pathExist(args[i]));
                //System.out.println(MiscUtil.pathExist(args[i], conf));
            //}

            //String value = conf.get("load.tbl.format");
            //if (value == null) {
                //System.out.printf("can not get value\n");
            //} else {
                //System.out.printf("load.tbl.format = %s\n", value);
            //}

            Path[] ps = MiscUtil.listFiles(new Path(args[0]), conf);
            for(Path p: ps) {
                System.out.println(p.toString());
            }

            return 0;
        }

    }

    public static void main(String[] args) throws Exception
    {
        // ipdist_test(args);

        // date_test(args);

        //final Log LOG = LogFactory.getLog("main-test");
        //LOG.info("oooo hello oooo");
        //LOG.warn("what hello oooo");
        //LOG.debug("what hello oooo");
        
        //final Log LOG1 = LogFactory.getLog("");
        //LOG1.info("oooo hello oooo");
        //LOG1.warn("what hello oooo");
        //LOG1.debug("what hello oooo");

        //System.out.printf(DateUtil.getYMDFormat("1339037811"));
        //System.out.printf(DateUtil.getYMDFormat("0"));

        //String s="/tmp/output/jointest/mapjoin-outer-2591";
        //System.out.println(MiscUtil.pathExist(s));

        int ret = ToolRunner.run(new Configuration(), new TestTools(), args);
        System.exit(ret);

        //TblColsParser ps = new TblColsParser();
        //ps.setSkip(1);
        //ps.setNullDefault("0");
        //ps.setContent("");
        //MiscUtil.printArray(ps.getGroupArray());
        //ps.setContent("ab\tcd");
        //MiscUtil.printArray(ps.getGroupArray());
        //ps.setContent("ab\t\t");
        //MiscUtil.printArray(ps.getGroupArray());

    }
}
