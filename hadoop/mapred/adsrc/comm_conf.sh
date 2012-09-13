
#export PATH=":$PATH"
#export CLASSPATH="./*:$CLASSPATH"

### ads data base dir
ADS_PATH=/home/lc/hadoop/testdata/ads
### web log dir
ACCESSLOG_PATH=$ADS_PATH/access-log
### account register log dir
REGISTER_PATH=$ADS_PATH/account/register
### account login log dir
LOGIN_PATH=$ADS_PATH/account/login
### game online login log dir
ONLINE_PATH=$ADS_PATH/account/online
### mb product consumption record
MB_PATH=$ADS_PATH/boss/mb
### vip record
VIP_PATH=$ADS_PATH/boss/vip
### all output's base dir
OUTPUT_PATH=$ADS_PATH/output
### all result data stored mysql's uri
RESULT_MYSQLURI='mysql://root:ta0mee@10.1.1.60/db_ads?useUnicode=true&characterEncoding=utf8'

# map ::= tmcid <=> tad
CONF_TADMAP_FILES='-files conf/tadmap.conf'
# map ::= productid <=> gameid
CONF_PRDMAP_FILES='-files conf/prdmap.conf'

OUTPUT_MAP_COMPRESS='-Dmapred.compress.map.output=true'
OUTPUT_REDUCE_COMPRESS='-Dmapred.output.compress=true -Dmapred.output.compression.codec=org.apache.hadoop.io.compress.GzipCodec'
JAR_NAME=AdsMon.jar
CONF_PARAM='-conf conf/hadoop-lc.xml'
HADOOP_FS="hadoop fs $CONF_PARAM "
HADOOP_JAR="hadoop jar $JAR_NAME "
LOADMYSQL="./load_shell.sh"
