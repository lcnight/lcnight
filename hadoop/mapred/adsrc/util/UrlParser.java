package util;

import java.util.*;

public class UrlParser
{
    private String murl = null;
    //private String mhost = null;
    //private String mpath = null;
    public UrlParser() {}

    // http://www.4399.com/abc.html?a=12&b=xx
    public void init(String url) {
        murl = url;
    }

    public String getHostPath() 
    {
        // remove schema token
        int beg = murl.indexOf("://") + 3; 
        if (beg < 5) {
            System.err.printf("error proto schema: %s\n", murl);
            return null;
        }

        String url = null;
        int end = murl.indexOf("?", beg);
        if (end == -1) {
            if (murl.endsWith("/")) {  // remove end "/"
                end = murl.length() - 1;
                url = murl.substring(beg, end);
            } else {
                url = murl.substring(beg);
            }
        } else {
            if (murl.charAt(end - 1) == '/') {  // remove end "/"
                end = end - 1;
            }
            url = murl.substring(beg, end);
        }
        return url;
    }

}
