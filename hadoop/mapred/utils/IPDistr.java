import java.util.*;
import java.sql.*;

public class IPDistr
{
    private class IPRange implements Comparable<IPRange>
    {
        private long startIP;
        private long endIP;
        private int provinceCode;
        private int cityCode;

        public long getStartIP()
        {
            return this.startIP;
        }

        public long getEndIP()
        {
            return this.endIP;
        }

        public int getProvinceCode()
        {
            return this.provinceCode;
        }

        public int getCityCode()
        {
            return this.cityCode;
        }

        public IPRange(long startIP, long endIP, int provinceCode, int cityCode)
        {
            this.startIP = startIP;
            this.endIP = endIP;
            this.provinceCode = provinceCode;
            this.cityCode = cityCode;
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

    private class IPRangeComparator implements Comparator<IPRange>
    {
        public int compare(IPRange first, IPRange second)
        {
            return first.compareTo(second);
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

    private Connection mysqlConn = null;
    private IPRange[] ipRangeList = null;
    private HashMap<Integer, String> provinceNameMap = new HashMap<Integer, String>();
    private HashMap<Integer, String> cityNameMap = new HashMap<Integer, String>();

    public IPDistr(Connection mysqlConn) throws SQLException
    {
        this.mysqlConn = mysqlConn;

        Statement statement = mysqlConn.createStatement();
        ResultSet result = null;

        ArrayList<IPRange> list = new ArrayList<IPRange>();

        result = statement.executeQuery(
                "SELECT start_ip, end_ip, province_code, city_code FROM t_city_ip");
        while (result.next()) {
            long startIP = result.getLong(1);
            long endIP = result.getLong(2);
            int provinceCode = result.getInt(3);
            int cityCode = result.getInt(4);

            list.add(new IPRange(startIP, endIP, provinceCode, cityCode));
        }

        ipRangeList = new IPRange[list.size()];
        list.toArray(ipRangeList);
        Arrays.sort(ipRangeList, new IPRangeComparator());


        result = statement.executeQuery(
                "SELECT province_code, province_name FROM t_province_code");
        while (result.next()) {
            int provinceCode = result.getInt(1);
            String provinceName = result.getString(2);
            provinceNameMap.put(provinceCode, provinceName);
        }

        result = statement.executeQuery("SELECT city_code, city_name FROM t_city_code");
        while (result.next()) {
            int cityCode = result.getInt(1);
            String cityName = result.getString(2);
            cityNameMap.put(cityCode, cityName);
        }
    }

    /**
     * @brief construct function, receive one mysql schema uri
     * @param  mysqluri ::= mysql://user:password@<host/ip>[:port]/database[?options]
     */
    public IPDistr(String mysqluri) throws ClassNotFoundException, SQLException
    {
        //String mysqlschema = "mysql://";
        //if (!mysqluri.startsWith(mysqlschema)) {
            //throw new SQLException();
        //}
        //int beg = mysqlschema.length();
        //int end = mysqluri.indexOf(':', beg);
        //String myuser = mysqluri.substring(beg, end);

        //beg = end + 1;
        //end = mysqluri.indexOf('@', beg);
        //String mypwd = mysqluri.substring(beg, end);

        try {
            Class.forName("com.mysql.jdbc.Driver");
        } catch (ClassNotFoundException e) {
            System.err.println("not find mysql lib");
            e.printStackTrace();
            throw e;
        }

        //String jdbcmysqlconnstr = String.format("jdbc:%s%s", mysqlschema, mysqluri.substring(end + 1));
        MysqlUriParser myp = new MysqlUriParser(mysqluri);
        try {
            //this.mysqlConn = DriverManager.getConnection(jdbcmysqlconnstr, myuser, mypwd);
            this.mysqlConn = DriverManager.getConnection(myp.getJdbcURL(), myp.getUser(), myp.getPassword());
        } catch (SQLException e) {
            System.err.println("get connection failed");
            e.printStackTrace();
            throw e;
        }

        Statement statement = mysqlConn.createStatement();
        ResultSet result = statement.executeQuery(
                "SELECT start_ip, end_ip, province_code, city_code " + 
                "FROM t_city_ip force index(ip_index) order by start_ip, end_ip");

        ArrayList<IPRange> list = new ArrayList<IPRange>();
        while (result.next()) {
            long startIP = result.getLong(1);
            long endIP = result.getLong(2);
            int provinceCode = result.getInt(3);
            int cityCode = result.getInt(4);

            list.add(new IPRange(startIP, endIP, provinceCode, cityCode));
        }
        ipRangeList = new IPRange[list.size()];
        list.toArray(ipRangeList);

        result = statement.executeQuery("SELECT province_code, province_name FROM t_province_code");
        while (result.next()) {
            int provinceCode = result.getInt(1);
            String provinceName = result.getString(2);
            provinceNameMap.put(provinceCode, provinceName);
        }

        result = statement.executeQuery("SELECT city_code, city_name FROM t_city_code");
        while (result.next()) {
            int cityCode = result.getInt(1);
            String cityName = result.getString(2);
            cityNameMap.put(cityCode, cityName);
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
            if (ipHost >= ipRangeList[mid].getStartIP()) {
                low = mid + 1;
            } else {
                high = mid - 1;
            }
        }

        if (high < 0 || high >= ipRangeList.length) {
            return null;
        }

        IPRange hit = ipRangeList[high];
        if (ipHost < hit.getStartIP() || ipHost > hit.getEndIP()) {
            return null;
        }

        return hit;
    }

    public int getIPProvinceCode(long ip)
    {
        IPRange ipRange = getHitRange(ip);

        if (ipRange == null) {
            return 0;
        }

        return ipRange.getProvinceCode();
    }

    public int getIPProvinceCode(String ipStr)
    {
        long ip = 0;
        try { 
            ip = Long.valueOf(ipStr);
        } catch (NumberFormatException e) {
            e.printStackTrace();
            return 0;
        } 
        
        IPRange ipRange = getHitRange(ip);
        if (ipRange == null) {
            return 0;
        }

        return ipRange.getProvinceCode();
    }

    public String getIPProvinceName(long ip)
    {
        int provinceCode = this.getIPProvinceCode(ip);

        if (provinceCode == 0) {
            return "Unknown";
        }

        String provinceName = this.provinceNameMap.get(provinceCode);

        if (provinceName == null) {
            return "Unknown";
        }

        return provinceName;
    }

    public String getIPProvinceNameByCode(int provinceCode)
    {
        String provinceName = this.provinceNameMap.get(provinceCode);

        if (provinceName == null) {
            return "Unknown";
        }

        return provinceName;
    }

    public int getIPCityCode(long ip)
    {
        IPRange ipRange = getHitRange(ip);

        if (ipRange == null) {
            return 0;
        }

        return ipRange.getCityCode();
    }

    public String getIPCityName(long ip)
    {
        int cityCode = this.getIPCityCode(ip);

        if (cityCode == 0) {
            return "Unknown";
        }

        String cityName = this.cityNameMap.get(cityCode);

        if (cityName == null) {
            return "Unknown";
        }

        return cityName;
    }

    public String getIPCityNameByCode(int cityCode)
    {
        String cityName = this.cityNameMap.get(cityCode);

        if (cityName == null) {
            return "Unknown";
        }

        return cityName;
    }
}
