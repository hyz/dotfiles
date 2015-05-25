#
# export MY_USER=lindu MY_PASS=administrator MY_HOST=moon.kklink.com MY_PORT=9000

MY_DIR=`dirname $0`
. $MY_DIR/config
. $MY_DIR/functions.sh

unset MY_GWID

[ -n "$1" ] || exit 1

fmt="city={1}&zone={2}&BarName={3}&address={4}&longitude={5}&latitude={6}&introduction={7}&phone={8}&SessionId={11}"
filename=$1

for x in `python $MY_DIR/fmtline.py $filename 12 $fmt`; do
    GET "/initBarSession?$x"
done

# 1009
# 深圳市
# 坪山
# 撒旦发到东方东方似懂非懂第三方松岛枫司法所水电费是否sd敢达发个梵蒂冈的
# 电饭锅电饭锅第三方刚的撒个梵蒂冈动画版通过恢复规划规范化涣发大号东莞鬼地方 个地方发鬼地方东莞电饭锅东莞个 的
# 189.021524
# 145.154545
# 是东方闪电电饭锅电饭锅大幅度规划规范计划规划局韩国鸡火锅韩 好过好规范化个恢复规划风格化风格化规范化发规划法规发给飞发给风格化防火防盗发给发挥好发给风格化返回风格化发规划法规风格化 发给恢复规划风格化发规划法规环境规划局v刹出v型吃 打不过东方报才八佰伴个人那几个换个
# 18931215545
# 
# 电饭锅发到过放
# B1009
# +--------------+---------------+------+-----+---------+----------------+
# | id           | int(11)       | NO   | PRI | NULL    | auto_increment |
# | city         | varchar(20)   | NO   |     | NULL    |                |
# | zone         | varchar(20)   | NO   |     | NULL    |                |
# | BarName      | varchar(100)  | NO   |     | NULL    |                |
# | address      | varchar(200)  | NO   |     | NULL    |                |
# | longitude    | double        | NO   |     | NULL    |                |
# | latitude     | double        | NO   |     | NULL    |                |
# | introduction | varchar(1000) | YES  |     | NULL    |                |
# | phone        | char(13)      | YES  |     | NULL    |                |
# | logo         | varchar(200)  | YES  |     | NULL    |                |
# | DevMacAddr   | char(7)       | YES  |     | NULL    |                |
# | SessionId    | varchar(20)   | NO   | PRI |         |                |
# +--------------+---------------+------+-----+---------+----------------+

# SessionId=KK1007A
# city=南昌
# zone=青云谱区
# BarName=南昌鬼子酒吧
# address=江西省南昌市青云谱区
# introduction=南昌人开的酒吧，极具南昌人的特色
# phone=15070017693
# longitude=120.245455
# latitude=145.12134
# 
# GET "/initBarSession?city=$city&zone=$zone&BarName=$BarName&address=$address&longitude=$longitude&latitude=$latitude&introduction=$introduction&phone=$phone&SessionId=$SessionId"
# 
