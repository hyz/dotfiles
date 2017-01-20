//网站的主域名
var serverURL = 'http://www.liuyoo.com:6060/index.php/Api';

/*
写入loclstorage缓存
 * @param key
 * @param data
*/
function setItem(key,data){
	localStorage.setItem(key,data);
}

function Get(name)
{
     var reg = new RegExp("(^|&)"+ name +"=([^&]*)(&|$)");
     var r = window.location.search.substr(1).match(reg);
     if(r!=null)return  unescape(r[2]); return null;
}
/*
读取localstorage缓存
  * @param key 
  * @returns
*/
function getItem(key){
	return localStorage.getItem(key);
}

function DannyTrde(){
	this.user_name="";
	this.user_funds=0;
	this.user_id=0;
	
	};
///请求服务器数据
DannyTrde.prototype.send=function(url,callback){
 // $.afui.showMask('数据加载中...');
 $.getJSON(serverURL+'/'+url+'&jsoncallback=?',{},function(data){
	
	callback(data);
	 //$.afui.hideMask();
 });	
	
	}

DannyTrde.prototype.DataServer={
 Start:function(url){
	this.DataUrl=url;
	this.ws = new WebSocket(url); 
	this.events=[]; 
	var me=this;
this.ws.onmessage = function(evt)
{
  
  var data=evt.data.split('|');
 // console.log(data);
 for(i=0;i<me.events.length;i++){
	  //console.log(11);
	if(me.events[i][0]==data[0]){
		 
		me.events[i][1](data[1]);  
		break;
		 }
	 
	 }
}; 
	 },	
 On:function(events,callback){
  this.events.push([events,callback]);	  
  
	 },
  Send:function(cmd,data){
	   this.ws.send(cmd+"|"+data);
	  
	  },
 
	
	};
function showline(code){
      setItem("code",code);
	  $.afui.loadContent("#trde",false,false,'flip');
	
	};
$.afui.autoLaunch = false;
$.afui.ready(function() {
	$(document.body).get(0).className = "eryuan";
	$.afui.useOSThemes = true;
	$.afui.setBackButtonVisibility(false)
});
///setItem('trde_time','1分钟');
var t=new Date();
var web = new DannyTrde();
web.DataServer.Start('ws://www.liuyoo.com:8081');
web.send("get_code_list?time="+getItem('time_type'),
function(msg) {
	$("#select_time li").each(function(){
	 if($(this).attr('time')==getItem('time_type')){
	      $("#select_time li").removeClass('ac');
		 $(this).addClass('ac');
	 }
	});
	var htmls = "";
	for (i = 0; i < msg.data.length; i++) {
		htmls += '<li total_time="'+msg.data[i].total_time+'" class_id="'+msg.data[i].class_id+'" id="c' + msg.data[i].code + '" user_code="'+ msg.data[i].user_code+'" bl="'+msg.data[i].bl+'"><em class="name">' + msg.data[i].name + '</em><em class="code">' + msg.data[i].user_code + '</em><em class="price">--</em><em class="zd">--</em></li>'
	}
	$("#trde_list").html(htmls)
	$("#trde_list li").bind("click",function(){
		setItem("code",$(this).attr("id").replace("c",""));
		setItem("code_name",$(this).find('em.name').text());
		setItem("user_code",$(this).attr("user_code"));
		setItem("bl",$(this).attr("bl"));
		setItem("class_id",$(this).attr("class_id"));
		setItem("total_time",$(this).attr("total_time"));
	     $.afui.loadContent("#trde",false,false,'flip');
		 
		});
}); window.addEventListener("load",
function() {
	FastClick.attach(document.body)
},
false);
$(document).ready(function() {
	
	if(getItem('user_type')==1){
					  $("#user_nick").html("模拟");
					   
					 }else{
					   $("#user_nick").html(getItem('user_nick'));
					  
						 }

		$("#funds_user").html(getItem('funds'));
	$("#reg_submit2").bind('click',function(){
		var er=false;
		$("#reg_form input").each(function(){
			if($(this).val()==''){
				$.MsgBox.Tip('error', $(this).attr('placeholder')+'不能为空');	
				 er=true;
				return false;
				}
			if($(this).attr('reg')!=''){
				  var reg = new RegExp($(this).attr('reg'));
				  if(!reg.test($(this).val())){
					  if($(this).attr('msg')){
						    $.MsgBox.Tip('error', $(this).attr('msg'));
							
						  }else{
						  
					  $.MsgBox.Tip('error', $(this).attr('placeholder')+'格式不正确');
					  
					  }	
					  er=true;
					  return false;
					  }
				}
			
			});
			if(er==true){return false;}
		var data='?yzm='+$('#iyanz').val()+'&mob='+$("#imob").val()+'&user_nick='+$('#iuser_name').val()+'&email='+$('#iemail').val();
		data+="&weixin="+$("#iweixin").val()+'&user_pass='+$('#ipass').val()+'&bank_card='+$('#ibank').val()+"&bank="+$('#ibank_name').val();
		data+="&safe_pass="+$("#ifundspass").val();
		data+="&id="+Get("id");
		
		web.send('reg'+data,function(msg){
			
			$.MsgBox.Tip('success', msg.data);	
			});
		});
		if(getItem("id")==null|getItem('id')==0){
	$("#mainvie").hide();
	$("#login").show();
	 
	}
	$("#btnrepass_submit").bind('click',function(){
		
		if($("#old_pass").val()==''){
			$.MsgBox.Tip('error', '旧密码不能为空');	
			return false;
			};
		if($("#pass").val()==''){
			$.MsgBox.Tip('error', '新码不能为空');	
			return false;
			};
		if($("#com_pass").val()==''){
			$.MsgBox.Tip('error', '确认密码密码不能为空');	
			return false;
			};
		if($("#pass").val()!=$("#compass").val()){
			$.MsgBox.Tip('error', '新密码和确认密码不一致!');
			return false;	
			}
		web.send('repass?member_id='+getItem('id')+'&old_pass='+$('#old_pass').val()+'&pass='+$('#pass').val(),function(msg){
			$.MsgBox.Tip('success', msg.data);	
			});
		
		});
		$("#safe_submit").bind('click',function(){
		
		if($("#s_old_pass").val()==''){
			$.MsgBox.Tip('error', '旧密码不能为空');	
			return false;
			};
		if($("#s_pass").val()==''){
			$.MsgBox.Tip('error', '新码不能为空');	
			return false;
			};
		if($("#s_com_pass").val()==''){
			$.MsgBox.Tip('error', '确认密码密码不能为空');	
			return false;
			};
		if($("#s_pass").val()!=$("#s_compass").val()){
			$.MsgBox.Tip('error', '新密码和确认密码不一致!');
			return false;	
			}
		web.send('safe_pass?member_id='+getItem('id')+'&old_pass='+$('#s_old_pass').val()+'&safe_pass='+$('#s_pass').val(),function(msg){
			$.MsgBox.Tip('success', msg.data);	
			});
		
		});
	$("#select_time li").bind('click',function(){
		setItem('trde_time',$(this).text());
		setItem('time_type',$(this).attr('time'));
		$("#select_time li").removeClass('ac');
		$(this).addClass('ac');
		web.send('get_code_list?time='+$(this).attr('time'),function(msg){
		 var htmls = "";
	for (i = 0; i < msg.data.length; i++) {
		htmls += '<li bl="'+msg.data[i].bl+'" total_time="'+msg.data[i].total_time+'" class_id="'+msg.data[i].class_id+'" id="c' + msg.data[i].code + '"><em class="name">' + msg.data[i].name + '</em><em class="code">' + msg.data[i].user_code + '</em><em class="price">--</em><em class="zd">--</em></li>'
	}
	
	$("#trde_list").html(htmls)
	$("#trde_list li").bind("click",function(){
		setItem("code",$(this).attr("id").replace("c",""));
		setItem("code_name",$(this).find('em.name').text());
        setItem("code",$(this).attr("id").replace("c",""));
		setItem("code_name",$(this).find('em.name').text());
		setItem("user_code",$(this).attr("user_code"));
		setItem("bl",$(this).attr("bl"));
		setItem("class_id",$(this).attr("class_id"));
		
		setItem("total_time",$(this).attr("total_time"));
	     $.afui.loadContent("#trde",false,false,'flip');
		});
			
			
			});
		
		});
		$("#buy_up").bind('click',function(){
			var html='<div id="trde_tip"><ul><li>到期时间:<em>'+$("#times option:selected").text()+'</em></li><li>投资金额:￥<em>'+$("#funds input").val()+'</em></li></ul><ul><li>交易品种:<em>'+getItem('code_name')+'</em></li><li>收益比例:<em>'+getItem('bl')*100+'%</em></li></ul><ul><li>委托方向:<em>买涨</em></li><li>最新价格:<em class="trde_price_n">'+$("#new_price").text()+'</em></li></ul></div>';
			$.afui.popup( {
   title:"买涨交易确认",
   message:html,
   cancelText:"取消",
   cancelCallback: function(){console.log("cancelled");},
   doneText:"确定",
   doneCallback: function(){
	   var data="member_id="+getItem('id')+
	   "&code="+getItem("code")+
	   "&user_code="+getItem("user_code")+
	   "&name="+getItem("code_name")+
	   "&funds="+$("#funds input").val()+
	   "&price="+$("#new_price").text()+
	   "&path=0"+
	   "&bl="+getItem("bl")+
	   "&e_type="+getItem("e_type")+
	     "&time_num="+$("#times").val();
	   web.send('buy?'+data,function(msg){
		    $.MsgBox.Tip('success',msg.data);
		     web.send('user_funds?member_id='+getItem('id'),function(msg){
				 setItem('funds',msg.data);
				 $("#funds_user").html("");
				 $("#funds_user").html(msg.data);
				 });
		   });
	   },
   cancelOnly:false
 });
			
			});
	$("#buy_down").bind('click',function(){
			var html='<div id="trde_tip"><ul><li>到期时间:<em>'+$("#times option:selected").text()+'</em></li><li>投资金额:￥<em>'+$("#funds input").val()+'</em></li></ul><ul><li>交易品种:<em>'+getItem('code_name')+'</em></li><li>收益比例:<em>'+getItem('bl')*100+'%</em></li></ul><ul><li>委托方向:<em>买跌</em></li><li>最新价格:<em class="trde_price_n">'+$("#new_price").text()+'</em></li></ul></div>';
			$.afui.popup( {
   title:"买跌交易确认",
   message:html,
   cancelText:"取消",
   cancelCallback: function(){console.log("cancelled");},
   doneText:"确定",
   doneCallback: function(){
	   var data="member_id="+getItem('id')+
	   "&code="+getItem("code")+
	   "&user_code="+getItem("user_code")+
	   "&name="+getItem("code_name")+
	   "&funds="+$("#funds input").val()+
	   "&price="+$("#new_price").text()+
	   "&path=1"+
	   "&bl="+getItem("bl")+
	   "&e_type="+getItem("e_type")+
	   "&time_num="+$("#times").val();
	   web.send('buy?'+data,function(msg){
		    $.MsgBox.Tip('success',msg.data);
		   web.send('user_funds?member_id='+getItem('id'),function(msg){
				 setItem('funds',msg.data);
				 $("#funds_user").html("");
				  $("#funds_user").html(msg.data);
				 });
		   });
	   },
   cancelOnly:false
 });
			
			});
	 $('#left li').each(function(){
            $(this).click(function(){
			$('#left li').removeClass('m_ac');
			 $(this).addClass('m_ac');
			   if($(this).attr('page')=="exit"){
               setItem('id',0);
			   $("#mainvie").hide();
	           $("#login").show();
			   }else{
                $.afui.loadContent('#'+$(this).attr('page'),false,false,'flip');
				
				}
				
            });
        });
	var line = new MinuteLine("#line");
	web.DataServer.On("data_line",function(msg){
		var data=msg.split(',');
		
		if (data[0] == getItem("code")){
			console.log(data);
				line.Update({price:data[1],time:data[2]});
			}
	
		
		});
	web.DataServer.On("data",
	function(data) {
		var datas = data.split(',');
		$(".p"+datas[0]).html("最新价:"+datas[1]);
		if (datas[0] == getItem("code")) {
			line.UpdatePrice(datas[1]);
			$("#new_price").html(datas[1]);
			
		    $(".trde_price_n").html(datas[1]);
		};
		$("#c" + datas[0] + " .price").html(datas[1]);
		$("#c" + datas[0] + " .zd").html(datas[2]);
		var zd = parseFloat(datas[2]);
		if (zd > 0) {
			$("#c" + datas[0] + " .price,#c" + datas[0] + " .zd").css({
				"color": "red"
			})
		} else {
			$("#c" + datas[0] + " .price,#c" + datas[0] + " .zd").css({
				"color": "green"
			})
		}
	});
	
	$("#btnlogin_submit").click(function() {
		
			if($("#txtlogin_uid").val()==''){
				$.MsgBox.Tip('error', '请输入登录账户！');
					return;
				}
			if($("#txtlogin_pwd").val()==''){
				$.MsgBox.Tip('error', '请输密码！');
				return;
				}
		web.send("login?user_name="+$("#txtlogin_uid").val()+"&user_pass="+$("#txtlogin_pwd").val(),function(data){
			
			if(typeof(data.data)=='string'){
				$.MsgBox.Tip('error', data.data);
				}else{
				 	
		         setItem("id",data.data.id);
				 setItem('funds',data.data.funds);
				 setItem('user_type',data.data.tp);
				 if(data.data.tp==1){
					  $("#user_nick").html("模拟");
					   setItem('user_nick',"模拟");
					 }else{
					   $("#user_nick").html(data.data.user_nick);
					    setItem('user_nick',data.data.user_nick);
						 }
				  $("#funds_user").html(data.data.funds);
				 $.MsgBox.Tip('success', '登录成功');	
					$('#login').hide();
		            $("#mainvie").show();
					
					}
			
			
			});
		
	
	});
	$('#send_yzm').click(function(){
		if($("#imob").val()==''){
			$.MsgBox.Tip('error', '请输入手机号码');
			return;
			}
		web.send('sendmob?mob='+$("#imob").val(),function(msg){
			 $.MsgBox.Tip('success', msg.data);
			});
		
		});
	$("#reg_submit").click(function() {
		
		
		$('#login').hide();
		$("#mainvie").hide()
		$('#reg').show();
	});
	$("#btnlogin_submit2").click(function(){
		$('#reg').hide();
		$("#mainvie").hide()
		$('#login').show();
		
		});
	$("#trde").bind("panelload",
	function() {
		$("#trde_title").html(getItem('code_name')+'('+getItem('code')+')');
		$("#new_price").html("0");
		var times=getItem('trde_time');
		if(times=="1分钟"||times==null){
			$("#times").html('<option value="60">1分钟</option>');
			setItem("e_type",0);
			}
			$("#shouyi").html((getItem('bl')*100)+"%");
		if(times=="5分钟"){$("#times").html('<option value="300">5分钟</option><option value="360">6分钟</option><option value="420">7分钟</option><option value="480">8分钟</option><option value="540">9分钟</option><option value="600">10分钟</option><option value="660">11分钟</option><option value="720">12分钟</option><option value="780">13分钟</option><option value="840">14分钟</option><option value="900">15分钟</option>');setItem("e_type",1);}
		if(times=="30分钟"){$("#times").html('<option value="1800">30分钟</option>');setItem("e_type",2);}
		if(times=="60分钟"){$("#times").html('<option value="3600">60分钟</option>');setItem("e_type",3);}
		if(getItem("code")!=null){
		web.send("get_line?code="+getItem("code")+"&name="+getItem('code_name'),function(msg){
			line.setData(msg.data,getItem("class_id"),getItem("total_time"));
			//alert(getItem("total_time"));
			});
		
		}
	});
	$("#btn_reuserinfo").bind('click',function(){
		$("#re_info input").each(function(){
			if($(this).val()==''){
				$.MsgBox.Tip('error', $(this).attr('placeholder')+'不能为空');	
				
				return false;
				}
			if($(this).attr('reg')!=''){
				  var reg = new RegExp($(this).attr('reg'));
				  if(!reg.test($(this).val())){
					  if($(this).attr('msg')){
						    $.MsgBox.Tip('error', $(this).attr('msg'));
							return false;
						  }else{
						  
					  $.MsgBox.Tip('success', $(this).attr('placeholder')+'格式不正确');
					  
					  }	
					 
					  return false;
					  }
				}
			
			});
		var data='?member_id='+getItem('id')+'&mob='+$("#x_mob").val()+'&user_nick='+$('#x_user_name').val()+'&email='+$('#x_email').val();
		data+="&weixin="+$("#x_weixin").val()+'&bank_card='+$('#x_bank_card').val()+"&bank="+$('#x_bank').val();
	   web.send('re_userinfo'+data,function(msg){
		   $.MsgBox.Tip('success', msg.data);
		    $.afui.loadContent('#re_user_info',false,false,'fade');
		   });
		
		
		});
	 $("#del").click(function(){
		 var f= parseInt($("#funds input").val());
		 if(f>100){
			$("#funds input").val(f-100);
			 }
		 });
		 $("#add").click(function(){
		 var f= parseInt($("#funds input").val());
		 
			$("#funds input").val(f+100);
		
		 });
	$("#re_user_info").bind('panelload',function(){
		web.send("get_user_info?member_id="+getItem("id"),function(msg){
			var data=msg.data;
			$("#x_user_name").val(data.user_nick);
			$("#x_mob").val(data.mob);
			$("#x_id_card").val(data.id_card);
			$("#x_email").val(data.email);
			$("#x_bank_card").val(data.bank_card);
			$("#x_bank").val(data.bank);
		   $("#x_weixin").val(data.weixin);
			
			
			
			
			});
		});
	$("#user_info").bind("panelload",function(){
		web.send("get_user_info?member_id="+getItem("id"),function(msg){
			var html='<li><em>登录帐号:</em><em>'+msg.data.user_name+'</em></li>'+
		 '<li><em>手&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;机:</em><em>'+msg.data.mob+'</em></li>'+	
        '<li><em>姓&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;名:</em><em>'+msg.data.user_nick+'</em></li>'+
      
       ' <li><em>邮&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;箱:</em><em>'+msg.data.email+'</em></li>'+
        '<li><em>账户余额:</em><em>￥'+msg.data.funds+'</em></li>'+
		 '<li><em>微&nbsp;&nbsp;信&nbsp;&nbsp;号:</em><em>'+msg.data.weixin+'</em></li>'+
        '<li><em>银行帐号:</em><em>'+msg.data.bank_card+'</em></li>'+
        '<li><em>开&nbsp;&nbsp;户&nbsp;&nbsp;行:</em><em>'+msg.data.bank+'</em></li>';
			
			$("#user_info .list").html(html);
		
			});
		
		});
	$(".menu li").bind('click',function(){
		if($(this).attr('page')){
			  $.afui.loadContent('#'+$(this).attr('page'),false,false,'flip');
			
			}
		
		});
		$("#funds_submits").click(function(){
			var funds=$("#out_funds").val();
		    var pass=$("#out_pass").val();
			if(pass==""|funds<=0){
				  $.MsgBox.Tip('success', "出金金额不能小于0，资金密码不能为空");
				  return false;
				}
			
			web.send('out_funds?member_id='+getItem("id")+"&pass="+pass+"&funds="+funds,function(msg){
				  $.MsgBox.Tip('success', msg.data);
				$("#out_funds,#out_pass").val('');
				
				});
			
			});
			$("#pay_log").bind("panelload",function(){
				web.send('pay_log?member_id='+getItem("id"),function(msg){
			   
				
				 var htmls="";	
				for(i=0;i<msg.data.length;i++){
					 var zt=msg.data[i].sta==0?"未支付":"入金成功";
					  htmls+='<ul><li>'+msg.data[i].creat_time+'</li><li>'+msg.data[i].funds+'</li><li>'+zt+'</li></ul>';	
					}	
				$("#pay_log .log_content").html(htmls);	
				});
				});
		$("#out_log").bind("panelload",function(){
				web.send('out_log?member_id='+getItem("id"),function(msg){
			   
				
				 var htmls="";	
				for(i=0;i<msg.data.length;i++){
					 var zt="";
					 if(msg.data[i].sta==0){zt="申请已提交";};
					  if(msg.data[i].sta==1){zt="出金成功";};
					   if(msg.data[i].sta==2){zt="出金失败";};
					  htmls+='<ul><li>'+msg.data[i].creat_time+'</li><li>'+msg.data[i].funds+'</li><li>'+zt+'</li></ul>';	
					}	
				$("#out_log .log_content").html(htmls);	
				});
				});
	$("#hist_trde").bind("panelload",function(){
		
		if(getItem("id")!=null){
		web.send('get_user_trde?member_id='+getItem("id"),function(msg){
			$("#ye").html(msg.data.funds);
			$("#yk").html(msg.data.yk);
			$("#count_num").html(msg.data.count_num);
			setItem('funds',msg.data.funds);
			$("#funds_user").html(msg.data.funds);
			});
			
		web.send("hist?member_id="+getItem("id"),function(msg){
		var htmls="";
		for(i=0;i<msg.data.length;i++){
			var str=(msg.data[i].path==0?"买涨":"买跌");
			var c=(msg.data[i].path==0?"p_zd":"p_d");
			var yl2=(msg.data[i].funds*msg.data[i].bl);
			var yl_t=(msg.data[i].sta==1?'p_zd':'p_d');
			var yl=(msg.data[i].sta==1?"盈利:"+yl2:"亏损");
			htmls+=' <div class="pro_list"><div><li>'+msg.data[i].name+'</li><li class="'+c+'">'+str+'</li><li>'+msg.data[i].code+'</li></div><div class="p_infos"><ul><li><em>执行价格</em><em>'+
			+msg.data[i].price+'</em></li><li><em>到期价格:</em><em>'+msg.data[i].e_price+'</em></li></ul><ul><li><em>开始时间</em><em>'
			+msg.data[i].creat_time+'</em></li><li><em>到期时间</em><em>'+msg.data[i].e_time+'</em></li></ul></div><li class="trde_jg '+yl_t+'"><em>投资金额:'+msg.data[i].funds+'</em><em>'+yl+'</em></li></div>';
		
			}
			$("#hist_list").html(htmls);
				
			/*$("#git .pro_list").click(function() {
		$.afui.actionsheet('<a  onclick="alert(\'hi\');" >撤单</a>')
	});*/
			});
		}
		
		
		});
	setInterval(function(){
		web.send('user_funds?member_id='+getItem('id'),function(msg){
				 setItem('funds',msg.data);
				 $("#funds_user").html("");
				 $("#funds_user").html(msg.data);
				 });
		
		},3000);
	$("#git").bind("panelload",
	function() {
		
		if(getItem("id")!=null){
		web.send("git?member_id="+getItem("id"),function(msg){
		var htmls="";
		for(i=0;i<msg.data.length;i++){
			var str=(msg.data[i].path==0?"买涨":"买跌");
			var c=(msg.data[i].path==0?"p_zd":"p_d");
			htmls+=' <div class="pro_list"><div><li>'+msg.data[i].name+'</li><li class="'+c+'">'+str+'</li><li>'+msg.data[i].code+'</li></div><div class="p_infos"><ul><li><em>执行价格</em><em>'+
			+msg.data[i].price+'</em></li><li><em>投资金额</em><em>'+msg.data[i].funds+'</em></li></ul><ul><li><em>开始时间</em><em>'
			+msg.data[i].creat_time+'</em></li><li><em>到期时间</em><em>'+msg.data[i].e_time+'</em></li></ul></div><li><em class="new_price p'+msg.data[i].code+'">最新价:0.00</em><em class="daojishi"  time="'+msg.data[i].e_time+'" ></em></li></div>';
		
			}
			$("#pro_list").html(htmls);
				t.timer(".daojishi");
			/*$("#git .pro_list").click(function() {
		$.afui.actionsheet('<a  onclick="alert(\'hi\');" >撤单</a>')
	});*/
			});
		}
	});
	setTimeout(function() {
		$.afui.launch()
	},
	150)
});


