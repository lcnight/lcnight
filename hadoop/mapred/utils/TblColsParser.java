import java.util.ArrayList;

public class TblColsParser
{
    private static String forall = "-1";
    private static String curline = null;
    private String [] cols = null;
    private String nullDef = "0";
    // only process cols index >= skipIdx
    private int skipIdx = 0;
    // index need reguar not null value to 1
    private int regularIdx = -1;
    private ArrayList<String> arr1 = new ArrayList<String>();
    private ArrayList<String> arr2 = new ArrayList<String>();

    public TblColsParser() { };
    public TblColsParser(String tblline)
    {
        setContent(tblline);
    }
    public void setContent(String line)
    {
        curline = line;
        cols = line.split("\t", -1);
    }
    public void setNullDefault(String nd)
    {
        nullDef = nd;
    }
    public void setSkip(int idx)
    {
        skipIdx = idx;
    }
    public void setRegularIndex(int idx)
    {
        regularIdx = idx;
    }

    protected String getColValue(int idx, String value)
    {
        if (idx == regularIdx && value.length() != 0) {
            return "1";
        } 
        return value.length()!=0?value:nullDef;
    }

    public String[] getGroupArray()
    {
        if (cols.length <= skipIdx) {
            return new String[0];
        }
        //if (cols.length < 4) {
            //System.err.printf("%s cols < 4\n", curline);
        //}
        arr2.clear();
        arr1.clear();
        arr1.add(cols[skipIdx].length()==0?nullDef:cols[skipIdx]);
        arr1.add(forall);
        int lastArr = 1;
        for (int i = skipIdx + 1 ; i < cols.length ; ++i) {
            if (lastArr == 1) {
                for (int m = 0 ; m < arr1.size() ; ++m) {
                    arr2.add(String.format("%s,%s", arr1.get(m), 
                                getColValue(i, cols[i])));
                    arr2.add(String.format("%s,-1", arr1.get(m)));
                }
                arr1.clear();
                lastArr = 2;
            } else {
                for (int n = 0 ; n < arr2.size() ; ++n) {
                    arr1.add(String.format("%s,%s", arr2.get(n), 
                                getColValue(i, cols[i])));
                    arr1.add(String.format("%s,-1", arr2.get(n)));
                }
                arr2.clear();
                lastArr = 1;
            }
        }
        if (lastArr == 1) {
            return arr1.toArray(new String[0]);
        } else {
            return arr2.toArray(new String[0]);
        }
    }
}
