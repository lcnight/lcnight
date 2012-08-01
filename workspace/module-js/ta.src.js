/*********************************************************************
 *
 * Taomee Analytics Javascript
 *
 * @author    bianwei<bianwei@vip.qq.com>
 * @version   1.0.2
 * @build     $Id: ta.src.js 23865 2010-12-02 05:41:13Z bianwei $
 * @copyright TaoMee, Inc. Shanghai China. All rights reserved.
 *
 ********************************************************************/
;(function(){
    /**
     * Define BOM & DOM
     */
    var V = "1.0.2",
        U = "http://10.1.1.60:7002/",
        D = document,
        L = D.location,
        N = navigator,
        S = screen,
        W = window;
    /**
     * Define Gif Request Parameters Key
     */
    var GA = "__tm.gif",
        GB = "&tmb=",
        GC = "&tmc=",
        GE = "&tme=",
        GF = "&tmf=",
        GG = "&tmg=",
        GH = "&tmh=",
        GJ = "&tmj=",
        GO = "&tmo=",
        GP = "&tmp=",
        GR = "&tmr=",
        GS = "&tms=",
        GT = "&tmt=",
        GL = "&tml=",
        GU = "&tmu=",
        GV = "&tmv=",
        GX = "&tmx=";
    /**
     * Define Cookie Parameters Key
     */
    var CG = "__tmcg",
        CV = "__tmcv",
        CT = "__tmct";
    /**
     * 获取浏览器名称及版本
     *
     * @return string
     */
    var b = function(){
        var s,
            b,
            ua = N.userAgent.toLowerCase();
        if (s = ua.match(/msie ([\d.]+)/)) {
            b = "IE " + s[1];
        } else {
            if (s = ua.match(/firefox\/([\d.]+)/)) {
                b = "Firefox " + s[1];
            } else {
                if (s = ua.match(/chrome\/([\d.]+)/)) {
                    b = "Chrome " + s[1];
                } else {
                    if (s = ua.match(/opera.([\d.]+)/)) {
                        b = "Opera " + s[1];
                    } else {
                        if (s = ua.match(/version\/([\d.]+).*safari/)) {
                            b = "Safari " + s[1];
                        } else {
                            b = "other browser";
                        }
                    }
                }
            }
        }
        return b;
    },
    /**
     * 获取Cookie值,目前只包含GUID
     *
     * @return string
     */
    c = function(){
        var cookie_g = TMC.uguid(),
            cookie_v = TMC.visit(),
            cookie_t = TMC.times();
        if(cookie_g == "-" || cookie_v == "-" || cookie_t == "-"){
            return "-";                    // 读写Cookie失败
        }
        var c  = CG + "=" + cookie_g + ";";// 用户唯一标示符
            c += CV + "=" + cookie_v + ";";// 用户访问时间(f.l.s)
            c += CT + "=" + cookie_t + ";";// 用户访问次数
        return c;
    },
    /**
     * 获取浏览器编码
     *
     * @return string
     */
    e = function(){
        var e = D.characterSet
             || D.charset
             || "-";
        return e.toLowerCase();
    },
    /**
     * 获取flash版本
     *
     * @return string
     */
    f = function(){
        var f = "";
        if (N.plugins && N.plugins.length) {
            for (var ii = 0; ii < N.plugins.length; ii++) {
                if (N.plugins[ii].name.indexOf("Shockwave Flash") != -1) {
                    f = N.plugins[ii].description.split("Shockwave Flash ")[1];
                    break;
                }
            }
        } else {
            if (W.ActiveXObject) {
                for (var ii = 10; ii >= 2; ii--) {
                    try {
                        var fl = eval("new ActiveXObject('ShockwaveFlash.ShockwaveFlash." + ii + "');");
                        if (fl) {
                            f = ii + ".0";
                            break;
                        }
                    } catch (e) {}
                }
            }
        }
        if (f == "") {
            f = "flash player not installed";
        }
        return f;
    },
    /**
     * 获取GUID
     *
     * @return string
     */
    g = function(){
        return cookie.get(CG)
            || cookie.set(CG, com.guid(), 60 * 24 * 365 * 2)
            || '-';
    },
    /**
     * 获取页面主机地址
     *
     * @return string
     */
    h = function(){
        return L.hostname && L.hostname != ""
            ? L.hostname
            : "-";
    },
    /**
     * 获取是否启用java
     *
     * @return string
     */
    j = function(){
        return N.javaEnabled()
            ? "1"
            : "0"
    },
    /**
     * 获取浏览器语言
     *
     * @return string
     */
    l = function(){
        var l;
        if (N.systemLanguage) {
            l = N.systemLanguage
        } else {
            if (N.browserLanguage) {
                l = N.browserLanguage
            } else {
                if (N.language) {
                    l = N.language
                } else {
                    if (N.userLanguage) {
                        l = N.userLanguage
                    } else {
                        l = "-"
                    }
                }
            }
        }
        return l.toLowerCase()
    },
    /**
     * 获取客户操作系统及版本
     *
     * @return string
     */
    o = function(){
        var os,
            ua = N.userAgent.toLowerCase(),
            pf = N.platform.toLowerCase();
        if (ua.indexOf("windows nt 5.1") > -1) {
            os = "Windows XP";
        } else {
            if (ua.indexOf("windows nt 6.1") > -1) {
                os = "Windows 7";
            } else {
                if (ua.indexOf("windows nt 6.0") > -1) {
                    os = "Windows Vista";
                } else {
                    if (ua.indexOf("windows nt 5.0") > -1) {
                        os = "Windows 2000";
                    } else {
                        if (ua.indexOf("windows nt 5.2") > -1) {
                            os = "Windows 2003";
                        } else {
                            if (pf.indexOf("linux") > -1) {
                                os = "Linux";
                            } else {
                                if (pf.indexOf("mac") > -1) {
                                    os = "Mac";
                                } else {
                                    if (pf.indexOf("iPhone") > -1) {
                                        os = "iPhone/iPod";
                                    } else {
                                        os = "other os";
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        return os;
    },
    /**
     * 获取页面请求路径
     *
     * @return string
     */
    p = function(){
        return (L.pathname + L.search) || "-";
    },
    /**
     * 获取来源URL
     *
     * @return string
     */
    r = function(){
        return D.referrer || "-";
    },
    /**
     * 获取屏幕分辨率
     *
     * @return string
     */
    s = function(){
        return (S.width || "-") + "x" + (S.height || "-");
    },
    /**
     * 获取当前时间戳
     *
     * @return int
     */
    t = function(){
        return com.timestamp();
    },
    /**
     * 获取页面完整URL
     *
     * @return string
     */
    u = function(){
        return W.location.protocol
            + "//"
            + W.location.host
            + W.location.pathname
            + W.location.search
            + W.location.hash;
    },
    /**
     * 获取当前代码版本号
     *
     * @return string
     */
    v = function(){
        return V || "-";
    },
    /**
     * 扩展参数
     *
     * @return string
     */
    x = function(){
        return "-";
    },
    cookie = {
        /**
         * 设置Cookie值
         *
         * @param string name
         * @param string value
         * @param string expires
         * @param string path
         * @param string domain
         * @param string secure
         * @return string
         */
        set : function(name, value, expires, path, domain, secure) {
                  var today = new Date();
                  today.setTime( today.getTime() );
                  if ( expires ) {
                      expires = expires * 1000 * 60;
                  }
                  var expires = new Date( today.getTime() + (expires) );
                  D.cookie  = name + '=' + escape( value )
                      + ((expires) ? ';expires=' + expires.toGMTString() : '' )
                      + ((path)    ? ';path=' + path : ';path=/' )
                      + ((domain)  ? ';domain=' + domain : '' )
                      + ((secure)  ? ';secure' : '' );
                  return cookie.get(name) || "-";
        },
        /**
         * 获取Cookie值
         *
         * @param string name
         * @return string
         */
        get : function(name) {
            var cookieArray = D.cookie.split("; "); //得到分割的cookie名值对
            var cookie = new Object();
            for (var i = 0; i < cookieArray.length; i++) {
                var arr = cookieArray[i].split("="); //将名和值分开
                if (arr[0] == name) return unescape(arr[1]); //如果是指定的cookie，则返回它的值
            }
            return "";
        },
        /**
         * 删除Cookie值
         *
         * @param string name
         */
        del : function(name) {
            var date = new Date();
            var ms = 1 * 1000;
            date.setTime(date.getTime() - ms);
            D.cookie = name + "=no; expires=" + date.toGMTString(); //将过期时间设置为过去来删除一个cookie
        }
    },
    /**
     * 通用公共的函数组件库
     */
    com = {
        timestamp : function(){
            return Date.parse(new Date())/1000 || "-";
        },
        /**
         * 参数值字符串 作为 URI 组件进行编码
         *
         * @param string  s
         * @param boolean u
         * @return string
         */
        encode : function(s, u){
            if (typeof(encodeURIComponent) == 'function') {
                if (u) return encodeURI(s);
                else return encodeURIComponent(s);
            } else {
                return escape(s);
            }
        },
        /**
         * 生成GUID
         *
         * @return string
         */
        guid : function(){
            var g = "";
            for (var i = 1; i <= 32; i++){
                var n = Math.floor(Math.random()*16.0).toString(16);
                g +=   n;
                if((i==8)||(i==12)||(i==16)||(i==20))
                    g += "-";
            }
            return g;
        },
        /**
         * 绑定事件方法
         */
        addEvent : function(obj, type, fn) {
            if ( obj.attachEvent ) {
                obj['e' + type + fn] = fn;
                obj[type + fn] = function(){
                    obj['e' + type + fn]( window.event );
                };
                obj.attachEvent( 'on' + type, obj[type + fn] );
            } else {
                obj.addEventListener( type, fn, false );
            }
        },
        /**
         * 等待DOM加载
         */
        ready : function(f){
            dom = [];
            dom.done  = false;
            dom.timer = null;
            dom.ready = null;
            /**
             * 真正的ready函数
             */
            var _Ready = function(f){
                if( dom.done ){
                    // 假如DOM 已经加载, 则立即运行
                    return f();
                }
                if( dom.timer ){
                    // 把它加入待执行函数清单中
                    dom.ready.push(f);
                }else{
                    // 使用addEvent为页面加载完毕绑定一个事件, 以防它最先加载
                    com.addEvent(W, "load", _isReady);
                    // 初始化待执行函数的数组
                    dom.ready = [f];
                    // 尽可能快地检测DOM 是否可用
                    dom.timer = setInterval( _isReady, 13);
                }
            },
            /**
             * 检测是否ready
             */
            _isReady = function(){
                //  如果DOM已可用, 则忽略
                if( dom.done ){
                    return false;
                }
                // 检测若干函数和元素是否可用
                if(D && D.getElementById && D.getElementsByTagName && D.body){
                    // 如果可用则停止检测
                    clearInterval( dom.timer );
                    dom.timer = null;
                    // 执行所有正等待的函数
                    for ( var i = 0; i < dom.ready.length; i++){
                        dom.ready[i]();
                    }
                    dom.ready = null;
                    dom.done  = true;
                }
             };
             /**
              * 执行ready
              */
             _Ready(f)
        }
    },
    /**
     * 设置或者更新cookie值组
     *
     */
    TMC = {
        /**
         * cookie过期时间(分钟)
         */
        expires : 1051200,// 两年
        /**
         * 获取GUID
         *
         * @return string
         */
        uguid : function(){
            return cookie.get(CG)
                || cookie.set(CG, com.guid(), this.expires)
                || '-';
        },
        /**
         * 获取用户访问时间
         *
         * @return string
         */
        visit : function(){
            var content,
                timestamp = com.timestamp();
            if( !cookie.get(CV) ){
                content = timestamp + "." + timestamp + "." + timestamp;
                cookie.set(CV, content, this.expires);
            } else {
                var a_visit;
                content = cookie.get(CV);
                a_visit = content.split(".");
                content = a_visit[0] + "." + a_visit[2] + "." + timestamp;
                cookie.set(CV, content, this.expires);
            }
            return cookie.get(CV) || "-";
        },
        /**
         * 获取用户访问次数
         *
         * @return string
         */
        times : function(){
            var times = parseInt(cookie.get(CT) || 0) + 1;
            return cookie.set(CT, times , this.expires) || "-";
        }
    },
    /**
     * 初始化函数
     */
    init = function(url){
        url      = url || U;
        var img  = new Image(1, 1);
        img.src  = url
                 + GA + "?"
                 + GB + com.encode(b())
                 + GC + com.encode(c())
                 + GE + com.encode(e())
                 + GF + com.encode(f())
                 + GG + com.encode(g())
                 + GH + com.encode(h())
                 + GJ + com.encode(j())
                 + GO + com.encode(o())
                 + GP + com.encode(p())
                 + GR + com.encode(r())
                 + GS + com.encode(s())
                 + GT + com.encode(t())
                 + GL + com.encode(l())
                 + GU + com.encode(u())
                 + GV + com.encode(v())
                 + GX + com.encode(x());
        img.onload = null;
    };
    /**
     * 等待DOM加载完成立即初始化
     */
    com.ready(init);
})();
