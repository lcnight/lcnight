package util;

import java.util.*;
import java.sql.*;

public class IPCountryDistr
{
    private class IPRange implements Comparable<IPRange>
    {
        private long startIP;
        private long endIP;
        private String countryCode;

        public long getStartIP()
        {
            return this.startIP;
        }

        public long getEndIP()
        {
            return this.endIP;
        }

        public String getCountryCode()
        {
            return this.countryCode;
        }

        public IPRange(long startIP, long endIP, String code)
        {
            this.startIP = startIP;
            this.endIP = endIP;
            this.countryCode = code;
        }

        public int compareTo(IPRange other)
        {
            long otherStartIP = other.getStartIP();
            long otherEndIP = other.getEndIP();

            if (this.startIP > otherStartIP) {
                return 1;
            } else if (this.startIP == otherStartIP) {
                if (this.endIP > otherEndIP) {
                    return 1;
                } else if (this.endIP == otherEndIP) {
                    return 0;
                } else {
                    return -1;
                }
            } else {
                return -1;
            }
        }
    }

    public long convertToIPHost(long ip)
    {
        long result = 0;

        result |= (ip >> 24) & 0xFF;
        result |= ((ip >> 16) & 0xFF) << 8;
        result |= ((ip >> 8) & 0xFF) << 16;
        result |= (ip & 0xFF) << 24;

        return result;
    }

    private IPRange[] ipRangeList = null;
    private HashMap<String, String> countryNameMap = new HashMap<String, String>();
    private Connection mysqlConn = null;

    /**
     * @brief construct function, receive one mysql schema uri
     * @param  mysqluri ::= mysql://user:password@<host/ip>[:port]/database[?options]
     */
    public IPCountryDistr(String mysqluri) throws ClassNotFoundException, SQLException
    {
        try {
            Class.forName("com.mysql.jdbc.Driver");
        } catch (ClassNotFoundException e) {
            System.err.println("not find mysql lib");
            e.printStackTrace();
            throw e;
        }

        MysqlUriParser myp = new MysqlUriParser(mysqluri);
        try {
            this.mysqlConn = DriverManager.getConnection(myp.getJdbcURL(), myp.getUser(), myp.getPassword());
        } catch (SQLException e) {
            System.err.println("get connection failed");
            e.printStackTrace();
            throw e;
        }

        Statement statement = mysqlConn.createStatement();
        ResultSet result = statement.executeQuery(
                "SELECT start_ip,end_ip,code2 FROM t_country_ip order by start_ip, end_ip");

        ArrayList<IPRange> list = new ArrayList<IPRange>();
        while (result.next()) {
            long startIP = result.getLong(1);
            long endIP = result.getLong(2);
            String countryCode = result.getString(3);

            list.add(new IPRange(startIP, endIP, countryCode));
        }
        ipRangeList = new IPRange[list.size()];
        list.toArray(ipRangeList);

        result = statement.executeQuery("SELECT c2code, country_cn FROM t_code_country");
        while (result.next()) {
            String countryCode = result.getString(1);
            String countryName = result.getString(2);
            countryNameMap.put(countryCode, countryName);
        }
    }

    public IPRange getHitRange(long ip)
    {
        int low = 0;
        int high = ipRangeList.length - 1;
        int mid = 0;
        long ipHost = this.convertToIPHost(ip);

        while (low <= high) {
            mid = (low + high) / 2;
            if (ipHost >= ipRangeList[mid].getStartIP() 
                    && ipHost <= ipRangeList[mid].getEndIP() ) {
                return ipRangeList[mid];
            }
            else if (ipHost > ipRangeList[mid].getEndIP() ) {
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }
        return null;
    }

    public String getIPCountryCode(long ip)
    {
        IPRange ipRange = getHitRange(ip);

        if (ipRange == null) {
            return "0";
        }

        return ipRange.getCountryCode();
    }

    public String getIPCountryCode(String ipStr)
    {
        long ip = 0;
        try { 
            ip = Long.valueOf(ipStr);
        } catch (NumberFormatException e) {
            e.printStackTrace();
            return "0";
        } 
        
        IPRange ipRange = getHitRange(ip);
        if (ipRange == null) {
            return "0";
        }

        return ipRange.getCountryCode();
    }

    public String getIPCountryName(long ip)
    {
        String countryCode = this.getIPCountryCode(ip);

        if (countryCode.equals("0")) {
            return "0";
        }

        String countryName = this.countryNameMap.get(countryCode);

        if (countryName == null) {
            return "0";
        }

        return countryName;
    }

    public String getIPCountryNameByCode(String CountryCode)
    {
        String CountryName = this.countryNameMap.get(CountryCode);

        if (CountryName == null) {
            return "0";
        }

        return CountryName;
    }

}
