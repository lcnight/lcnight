import java.io.*;
import java.sql.*;
import java.util.*;

public class buildEnv
{
    //private static void Debug();
    private static String get_tmp_path(String prefix, String suffix) {
        String path = String.format("/tmp/%s-%s.%s", prefix, UUID.randomUUID().toString(), suffix);
        return path;
    }

    private static void PrintSQLEx(SQLException ex)
    {
        System.out.println("SQLException: " + ex.getMessage());
        System.out.println("ErrorCode: " + ex.getErrorCode());
        System.out.println("SQLState: " + ex.getSQLState());
    }

    private static int set_dim_table(String datafile, String db_opt) throws Exception 
    {
        Connection conn = null;
        int beg = 0;
        int end = db_opt.indexOf(':');
        String username = db_opt.substring(beg, end);
        beg = end + 1;
        end = db_opt.indexOf('@', beg);
        String password = db_opt.substring(beg, end);
        beg = end + 1;
        String host_db = db_opt.substring(beg);
        String conn_str = String.format("jdbc:mysql://%s?autoReconnect=true&useUnicode=true&characterEncoding=utf8", host_db);
        System.out.println("opts string: " + db_opt);
        System.out.println("conn string: " + conn_str);

        try { 
            Class.forName("com.mysql.jdbc.Driver").newInstance();
            conn = DriverManager.getConnection(conn_str, username, password);
            //conn = DriverManager.getConnection(conn_str);
        } catch (SQLException ex) {
            PrintSQLEx(ex);
            System.exit(0);
        } 

        System.out.println("Connect successfully ... ");

        BufferedReader in = new BufferedReader(new FileReader(datafile));
        String line;
        String table = "";
        TreeMap dim_map = new TreeMap();
        Statement stmt = conn.createStatement();
        while ((line=in.readLine()) != null) {
            String[] cols = line.split("\t");


            if (cols.length != 3) {
                System.err.printf("split %s get %d cols, not equal 3\n", line, cols.length);
                continue;
            }

            String dim = cols[0];
            String dim_id = String.format("%s_id", dim);
            String dim_name = String.format("%s_name", dim);
            String t_dim_tbl = String.format("t_dim_%s", dim);

            if (!table.equals(t_dim_tbl)) {
                table = t_dim_tbl;
                dim_map.clear();
                String query_str = String.format("select %s, %s from %s;", dim_id, dim_name, t_dim_tbl);
                System.out.printf("\nget from table: %s\n", t_dim_tbl);
                ResultSet rs = null;
                try { 
                    if (stmt == null) { stmt = conn.createStatement(); }
                    rs = stmt.executeQuery(query_str);
                    while (rs.next()) {
                        String col_dim_name = rs.getString(dim_name);
                        int col_dim_id= rs.getInt(dim_id);
                        System.out.printf("\t%s => %d\n", col_dim_name, col_dim_id);
                        dim_map.put(col_dim_name, new Integer(col_dim_id));
                    }
                } catch (SQLException ex) {
                    table = ""; // reset table to null
                    PrintSQLEx(ex);
                    if (ex.getErrorCode() == 1146) {
                        System.err.printf("ignore not existed dim table %s\n", t_dim_tbl);
                        continue;
                    }
                } finally {
                    if (rs != null) { rs.close(); }
                }
            }

            String dim_key = cols[1];
            Object obj = dim_map.get(dim_key);
            if (obj != null) {
                System.err.printf("exists key: '%s'\n", dim_key);
            } else {
                System.err.printf("insert key: '%s'\n", dim_key);
                try{
                    if (stmt == null) { stmt = conn.createStatement(); }
                    String in_sql = String.format("insert into %s(%s) values ('%s');", t_dim_tbl, dim_name, dim_key);
                    stmt.executeUpdate(in_sql);
                    System.out.println(in_sql);
                } catch (SQLException ex) {
                    PrintSQLEx(ex);
                    //if (stmt != null) { stmt.close(); }
                }
            }
        } // end of while

        if (stmt != null) {
            stmt.close();
        }
        if (conn != null) {
            conn.close();
        }

        return 0;
    }

    public static int setup_env(String output_dir, String db_opt) throws Exception
    {
        String tmp_file = get_tmp_path("wireless", "ph");
        String hadoop_merge_cmd = String.format("hadoop fs -getmerge %s %s", output_dir, tmp_file);

        System.out.printf("command: %s\n", hadoop_merge_cmd);
        Process process = Runtime.getRuntime().exec(hadoop_merge_cmd);
        BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(process.getInputStream()));
        String s;
        while((s=bufferedReader.readLine()) != null)
            System.out.println(s);
        BufferedReader bufferedReader2 = new BufferedReader(new InputStreamReader(process.getErrorStream()));
        while((s=bufferedReader2.readLine()) != null)
            System.err.println(s);

        process.waitFor();

        return set_dim_table(tmp_file, db_opt);
    }

    public static void echo(String a)
    {
        System.err.println(a);
    }
    ///*
    //public static void main(String args[]) throws Exception
    //{
        ////if (args.length < 2) {
            ////System.err.println("Usage: exe <output-dir> <db option string>\n"+
                    ////"db option string example:\n\tuser:password@ip[:port]/database");

            ////System.exit(1);
        ////}

        ////setup_env(args[0], args[1]);
        ////set_dim_table("/tmp/test.ph", "root:ta0mee@10.1.1.60/db_wireless_dim");
    //}
    //*/
}
