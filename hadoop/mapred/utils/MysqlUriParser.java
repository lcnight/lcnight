import java.util.*;
import java.sql.SQLException;

public class MysqlUriParser
{

    private String mysqlschema = "mysql://";
    private String myuser = null;
    private String mypwd = null;
    private String myjdbcurl = null;

    public MysqlUriParser() {
    }
    public MysqlUriParser(String uri) throws SQLException
    {
        setUri(uri);
    }

    public void setUri(String mysqluri) throws SQLException
    {
        if (!mysqluri.startsWith(mysqlschema)) {
            throw new SQLException();
        }
        int beg = mysqlschema.length();
        int end = mysqluri.indexOf(':', beg);
        myuser = mysqluri.substring(beg, end);

        beg = end + 1;
        end = mysqluri.indexOf('@', beg);
        mypwd = mysqluri.substring(beg, end);

        myjdbcurl = String.format("jdbc:%s%s", mysqlschema, mysqluri.substring(end + 1));
        //System.out.printf("conn str: %s\nuser:%s, pwd:%s\n", myjdbcurl, myuser, mypwd);
    }

    public String getUser() {
        return myuser;
    }
    public String getPassword() {
        return mypwd;
    }
    public String getJdbcURL() {
        return myjdbcurl;
    }
}
