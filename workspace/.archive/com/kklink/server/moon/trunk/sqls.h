#ifndef _SQLS_H_
#define _SQLS_H_

#define INSERT_USERS ("insert into users(UserName,password) values('%s','%s')")

#define SELECT_USERS1 ("select * from users where UserId=%s") 

#define SELECT_USERS_ID_PASSWORD1 ("select password from users where UserId=%ld")

#define SELECT_USERS_USERPHONE_PASSWORD ("select password from users where UserPhone='%s'")

#define SELECT_USERS_MAIL_PASSWORD ("select password from users where mail='%s'")

#define SELECT_USERS_NAME_PASSWORD ("select password from users where UserName='%s'")




#define UPDATE_USERS_PASSWORD2 ("update users set password = '%s' where UserId= %ld")

#define UPDATE_USERS_USERPHONE2 ("update users set UserPhone = '%s' where UserId=%s")

#define UPDATE_USERS_MAIL2 ("update users set mail = '%s' where UserId=%s")

#define SELECT_BARS_LIST4 ("select * from bars where city='%1%' and zone='%2%' and id>%3% limit %4%")

#define SELECT_BARS_HOLE_CITY_LIST3 ("select * from bars where city='%1%'and id>%2% limit %3%")

#define SELECT_BARS_BARNAME2 ("select * from bars where city='%s' and BarName like '%%%s%%'")

#define SELECT_BARS_CITY ("select distinct city from bars")

#define SELECT_BARS_ZONE1 ("select distinct zone from bars where city='%s'")


// #define INSERT_SESSIONS4 ("insert into sessions(CreatorId,SessionId,SessionName,SessionType) values(%1%, '%2%', '%3%','%4%')")
// #define UPDATE_SESSIONS4 ("UPDATE sessions set CreatorId=%1%,SessionName='%3%',SessionType='%4%' where SessionId='%2%'")
// #define UPDATE_SESSIONS_SESSIONNAME2 ("update sessions set SessionName='%s' where SessionId='%s'")


// #define INSERT_SESSIONMEMBERS2 ("insert SessionMembers(UserId,SessionId) values(%ld,'%s')")
// #define DELETE_SESSIONMEMBERS2 ("delete from SessionMembers where SessionId='%s' and UserId=%ld")
// #define DELETE_QUIT_SESSIONS1 ("delete from SessionMembers where UserId=%ld and (SessionId like 'P%%' or SessionId like 'B%%')")
// #define SELECT_SESSIONMEMBERS_EXCEPT_USERID2 ("select * from SessionMembers where SessionId = '%s' and UserId != %s")
// #define SELECT_SESSIONMEMBERS_USERID2 ("select * from SessionMembers where SessionId = '%s' and UserId = %ld")
// #define UPDATE_SESSIONMEMBERS_REJECTMSGFLAG3 ("update SessionMembers set RejectMsgFlag = %s where SessionId = '%s' and UserId = %ld")

// #define INSERT_MESSAGES ("insert into messages(SessionId,UserId,MsgType,content) values('%s', %s,'%s','%s')")

 #define SELECT_MESSAGES ("select * from messages where id=%s")


// #define INSERT_UNPUSHEDMESSAGES ("insert into UnpushedMessages(UserId,MsgId) values(%s,%s)")

// #define DELETE_UNPUSHEDMESSAGES ("delete from UnpushedMessages where UserId = %s and MsgId = %s")

#define SELECT_UNPUSHEDMESSAGES1 ("select * from UnpushedMessages where UserId=%ld")


#define SELECT_INDIVIDUALALBUM1 ("select * from IndividualAlbum where UserId=%s")

#define INSERT_INDIVIDUALALBUM3 ("insert into IndividualAlbum(UserId, id, url) values(%1%, %2%, '%3%')")

#define DELETE_INDIVIDUALALBUM2 ("delete from IndividualAlbum where id=%1% and UserId=%2%")


#define INSERT_INDIVIDUALDATAS1 ("insert into IndividualDatas(UserId) values(%s)")

#define SELECT_INDIVIDUALDATAS1 ("select UserId,money,RejectOutInvitation,PushStatus,background,flags,attention_bars from IndividualDatas where UserId=%1%")

#define UPDATE_INDIVIDUALDATAS_BACKGROUND2 ("update IndividualDatas set background='%s' where UserId = %s")

#define UPDATE_INDIVIDUALDATAS_DEVICETOKEN4 ("update IndividualDatas set DeviceType='%s',DeviceToken='%s',PushStatus=%s where UserId = %ld")

#define UPDATE_INDIVIDUALDATAS_PUSHSTATUS1 ("update IndividualDatas set PushStatus=2 where UserId = %ld")

#define UPDATE_INDIVIDUALDATAS_SPEND_MONEY2 ("UPDATE IndividualDatas set money=money-%1% WHERE UserId=%2%")


#define SELECT_BARLIVEALBUM3 ("select * from BarLiveAlbum where id<%1% and SessionId='%2%'order by id desc limit %3%")

#define INSERT_BARLIVEALBUM2 ("insert into BarLiveAlbum(SessionId,url) values('%1%', '%2%')")


#define SELECT_BARPARTIES1 ("select * from BarParties where SessionId='%1%'")

#define INSERT_PHOTOS ("insert into photos(UserId,url) values(%s, '%s')")


#define INSERT_WHISPERS2 ("insert into whispers(UserId,OtherId) values(%1%,%2%)")

#define SELECT_WHISPERS2 ("select * from whispers where UserId=%1% and OtherId=%2%")

#define DELETE_WHISPERS2 ("delete from whispers where UserId=%1% and OtherId=%2%")


#define INSERT_CONTACTS3 ("INSERT INTO contacts(UserId,OtherId,relation) VALUES(%1%,%2%,%3%)")

#define DELETE_CONTACTS2 ("delete from contacts where UserId=%1% and OtherId=%2%")

#define UPDATE_CONTACTS_RELATION3 ("update contacts set relation=%1% where UserId=%2% and OtherId=%3%")

#define UPDATE_CONTACTS_INSERT_RELATION3 ("INSERT INTO contacts(UserId,OtherId,relation) VALUES(%1%,%2%,%3%) ON DUPLICATE KEY UPDATE relation=%3%")

#define UPDATE_CONTACTS_RELATION_NOREJECT3 ("update contacts set relation=%1% where UserId=%2% and OtherId=%3% and relation!=2")

#define UPDATE_CONTACTS_OTHERNAME3 ("update contacts set OtherName='%1%' where UserId=%2% and OtherId=%3%")

#define UPDATE_CONTACTS_USERNAME3 ("update contacts set UserName='%1%' where UserId=%2% and OtherId=%3%")

#define UPDATE_CONTACTS_INSERT_USERNAME3 ("INSERT INTO contacts(UserId,OtherId,UserName,relation) VALUES(%1%,%2%,'%3%',3) ON DUPLICATE KEY UPDATE UserName='%3%'")

#define UPDATE_CONTACTS_DELETE_USERNAME2 ("update contacts set UserName='' where UserId=%1% and OtherId=%2%")

#define SELECT_CONTACTS_OTHERNAME3 ("select OtherName from contacts where UserId=%1% and OtherId=%2%")

#define SELECT_CONTACTS_FANS1 ("select * from contacts where OtherId=%1% order by admire_time desc")

#define SELECT_CONTACTS_FANS_STATISTIC1 ("select count(*) from contacts where OtherId = %1% and relation = 1")

#define SELECT_CONTACTS_ADMIRE1 ("select * from contacts where UserId = %1% order by admire_time desc")

#define SELECT_CONTACTS_ADMIRE_STATISTIC1 ("select count(*) from contacts where UserId = %1% and relation = 1")

#define SELECT_CONTACTS_RELATION2 ("select relation from contacts where UserId=%1% and OtherId=%2%")


#define INSERT_USERGIFTS3 ("insert into UserGifts(UserId,FromUserId,type) values(%1%,%2%,%3%)")

#define SELECT_USERGIFTS2 ("select * from UserGifts where id<%s and UserId = %s order by id desc")

#define SELECT_USERGIFTS_STATISTIC1 ("select type from UserGifts where UserId = %1%")


#define SELECT_GOODS_ALL ("select * from goods where gstatus=1")

#define SELECT_GOODS_ID1 ("select * from goods where id=%1%")


#define SELECT_VERSIONS ("select * from versions DeviceType = %s order by UploadTime limit 1") 


#define INSERT_FEEDBACKS4 ("insert into feedbacks values(%1%,'%2%','%3%',%4%)")

#define SELECT_GIFTS1 ("select * from gifts where GiftId=%s")


#define INSERT_EXCHANGERADDRESS7 ("insert into ExchangerAddress(UserName,UserPhone,zone,DetailAddress,goodId,indent_number,UserId) values('%1%','%2%','%3%','%4%',%5%,'%6%','%7%')")

#define DELETE_EXCHANGEGOODS3 ("DELETE from UserGifts WHERE UserId=%1% and type=%2% ORDER BY id LIMIT %3%") 

#define SELECT_MONEY ("select * from money")

#define SELECT_MONEY_ID1 ("select * from money where id=%1%")

// #define INSERT_STATISTICS_CHARISMA ("insert into statistics_charisma(UserId,FromId,type,SessionId) values(%1%,%2%,%3%,'%4%')")

#define UPDATE_ADVERTISING_DEFAULT ("update advertising set is_default=0 where UserId=%1% and id!=%2%")
#endif
