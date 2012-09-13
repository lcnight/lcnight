import java.util.*;

public class test
{
    public static void main(String args[]) throws Exception
    {
        AdParser adp = new AdParser("../../conf/tadmap.conf");

        //adp.init("7");
        Iterator<String> adit =  adp.iterator();
        //while (adit.hasNext()) {
            //System.out.println(adit.next());
        //}

        //System.out.printf("len %d\n", args.length);

        //adp.init("#http://www.4399.com/news/ab.html");
        //adp.init("#www.4399.com/");
        adp.init(args[0]);
        adit =  adp.iterator();
        while (adit.hasNext()) {
            System.out.println(adit.next());
        }


    }
}
