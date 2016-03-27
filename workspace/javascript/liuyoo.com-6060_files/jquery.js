(function() {

    $.MsgBox = {

        Tip: function(title, msg) {

            GenerateHtml("tip", title, msg, null);

            btnOk(); //alert只是弹出消息，因此没必要用到回调函数callback

            btnNo();

        },

        Tip: function(title, msg, callback) {

            GenerateHtml("tip", title, msg, callback);

            btnOk(callback); //alert只是弹出消息，因此没必要用到回调函数callback

            btnNo();

        },

        Alert: function(title, msg, callback) {

            GenerateHtml("alert", title, msg, null);

            btnOk(callback);

            //btnNo();

        },

        Confirm: function(title, msg, callback) {

            GenerateHtml("confirm", title, msg, null);

            btnOk(callback);

            btnNo();

        },
		Loading: function(title, msg, callback) {

            GenerateHtml("Loading", title, msg, null);

            btnOk();

            //btnNo();

        },
		Call: function(title, msg, callback) {

            GenerateHtml("call", title, msg, null);

            btnOk();
			
			btnNo();

        },
		Clean:function(){
			 $("#mb_box,#mb_con,#mb_bg").remove();
		}


    }

    //生成Html

    var GenerateHtml = function(type, title, msg, callback) {

        var _html = "";
		
		if(type == "Loading"){
			
			_html += '<div id="mb_bg" style="position:fixed; z-index:10000; height:100%; width:100%; top:0; left:0; ">';
			_html += '<div class="spinner-box" style="position:fixed; z-index:10001;">';
			_html += '<div class="spinner">';
			_html += '<div class="spinner-container container1">';
			_html += '<div class="circle1"></div>';
			_html += '<div class="circle2"></div>';
			_html += '<div class="circle3"></div>';
			_html += '<div class="circle4"></div>';
			_html += '</div>';
			_html += '<div class="spinner-container container2">';
			_html += '<div class="circle1"></div>';
			_html += '<div class="circle2"></div>';
			_html += '<div class="circle3"></div>';
			_html += '<div class="circle4"></div>';
			_html += '</div>';
			_html += '<div class="spinner-container container3">';
			_html += '<div class="circle1"></div>';
			_html += '<div class="circle2"></div>';
			_html += '<div class="circle3"></div>';
			_html += '<div class="circle4"></div>';
			_html += '</div>';
			_html += '</div>';
			_html += msg+'</div>';
			_html += '</div>';

			
		}

        if (type == "confirm") {

            _html += '<div id="mb_bg" style=" background:#000; position:fixed; z-index:10000; height:100%; width:100%; top:0; left:0; opacity:0.5;"></div>';

			if($(window).width()<350){
				_html += '<div id="mb_box" class="alertwin ajaxwin" style="background:#fff; position:fixed; top:50%;left:50%; width:90%; height:auto; z-index:10002;">';
			}else{
				_html += '<div id="mb_box" class="alertwin ajaxwin" style="background:#fff; position:fixed; top:50%;left:50%; width:90%; height:auto; z-index:10002;">';
			}

            _html += '<div style="float: right; font-size:2em; margin:0 2px; width:30px; height:30px; line-height:2em; text-align:center"  class="close click-ccc-aaa"><a id="mb_ico"><i class="fa fa-times"></i></a></div>';

            _html += '<div style="margin:0 0.5em 0 0.5em; text-align:left; font-size:2em;line-height:2em;border-bottom:1px solid #000;text-align:center" >' + title + '</div>';

            _html += '<div style="margin:1em 0.5em 1em 0.5em; text-align:left; font-size:1.3em" >' + msg + '</div>';

            _html += '<div class="buttondiv" style="margin:20px auto 0 auto; height:4em;text-align:center">';

            _html += '<button type="button" id="mb_btn_ok" style=" padding:0.5em;margin-left:auto;margin-right:auto;font-size:1.5em;background:#498DDE; color:#fff;text-align:center;-moz-border-radius: 0.5em;-webkit-border-radius: 0.5em;border-radius:6px;border:none;width:90%;" class="ok">确定</button>';

            //_html += '<button type="button" id="mb_btn_ok" style=" height:50px;font-size:14px;line-height:50px; float:left; text-align:center; width:50%; background:#498DDE;color:#fff" class="ok">确定</button>';

            //_html += '<button type="button" id="mb_btn_no" style=" height:50px;font-size:14px;line-height:50px; float:left; text-align:center; width:50%; background:#efefef" class="cancel">取消</button>';

            _html += '</div>';

            _html += '</div>';

        }
		
		  if (type == "call") {

            _html += '<div id="mb_bg" style=" background:#000; position:fixed; z-index:10000; height:100%; width:100%; top:0; left:0; opacity:0.5;"></div>';

			if($(window).width()<350){
				_html += '<div id="mb_box" class="alertwin ajaxwin" style="background:#fff; position:fixed; top:50%;left:50%; width:90%; height:auto; z-index:10002;">';
			}else{
				_html += '<div id="mb_box" class="alertwin ajaxwin" style="background:#fff; position:fixed; top:50%;left:50%; width:90%; height:auto; z-index:10002;">';
			}

            _html += '<div style="float: right; font-size:2em; margin:0 2px; width:30px; height:30px; line-height:2em; text-align:center"  class="close click-ccc-aaa"><a id="mb_ico"><i class="fa fa-times"></i></a></div>';
			
			_html += '<div style="margin:0 1em 0 1em; text-align:left; font-size:2em;line-height:2em;border-bottom:1px solid #000;text-align:center" >' + title + '</div>';

            _html += '<div style="margin:1em 0.5em 1em 0.5em; text-align:left; font-size:1.3em" >' + msg + '</div>';

            _html += '<div class="buttondiv" style="margin:1em auto 0 auto; height:4em; text-align:center">';

            _html += '<button type="button" id="mb_btn_ok" style=" padding:0.5em;margin-left:auto;margin-right:auto;font-size:1.5em;background:#498DDE; color:#fff;text-align:center;-moz-border-radius: 0.5em;-webkit-border-radius: 0.5em;border-radius:6px;border:none;width:90%;" class="ok">呼叫</button>';

            //_html += '<button type="button" id="mb_btn_no" style=" height:50px;font-size:14px;line-height:50px; float:left; text-align:center; width:50%; background:#efefef" class="cancel">取消</button>';

            _html += '</div>';

            _html += '</div>';

        }

        if (type == "alert") {

            _html += '<div id="mb_box" class="alertwin ajaxwin" style="background:#fff; position:fixed; top:50%;left:50%; width:350px; height:auto; z-index:100001;">';

            //_html += '<div style="float: right; font-size:14px; margin:0 2px; width:30px; height:30px; line-height:30px; text-align:center"  class="close click-ccc-aaa"><a id="mb_ico"><i class="fa fa-times"></i></a></div>';

            _html += '<div style="margin:50px 25px 20px 25px; text-align:center; font-size:16px" >' + msg + '</div>';

            _html += '<div class="buttondiv" style="margin:40px auto 0 auto; height:40px;">';

            _html += '<button type="button" id="mb_btn_ok" style=" height:50px;font-size:14px;line-height:50px; float:left; text-align:center; width:100%" class="bgcolor-EF8 btnbgimg color-FFF width-100p ok">确定</button>';

            _html += '</div></div>';

            _html += '<div id="mb_bg" style=" background:#000; position:fixed; z-index:10001; height:100%; width:100%; top:0; left:0; opacity:0.5;"></div>';

        }

        if (type == "tip") {

            if (title == 'success') {

                _html += '<div class="notifybar bg-green" style="background-color:#12C994; position:fixed; z-index:1000000; top:0; left:50%; width:100%; height:auto; line-height:1.8; padding:11px 30px; color:#fff; font-size:16px; text-align:center;">' + msg + '</div>';

            }

            if (title == 'error') {

                _html += '<div class="notifybar bg-yellow" style="background-color:#F06962; position:fixed; z-index:1000000; top:0; left:50%; width:100%; height:auto; line-height:1.8; padding:11px 30px; color:#fff; font-size:16px; text-align:center;">' + msg + '</div>';

            }

        }

        //_html += '<div id="mb_box"></div><div id="mb_con"><span id="mb_tit">' + title + '</span>';

        //_html += '<a id="mb_ico">x</a><div id="mb_msg">' + msg + '</div><div id="mb_btnbox">';

        //if (type == "alert") {

        // _html += '<input id="mb_btn_ok" type="button" value="确定" />';

        //}

        //if (type == "confirm") {

        //_html += '<input id="mb_btn_ok" type="button" value="确定" />';

        //_html += '<input id="mb_btn_no" type="button" value="取消" />';

        //}

        //_html += '</div></div>';

        //必须先将_html添加到body

        $("body").append(_html);

        //设置Css样式

        GenerateCss(type);
		
		if (type == "Loading") {
			var loadingTop=($(window).height()-$('.spinner-box').height())/2;
			var loadingLeft=($(window).width()-$('.spinner-box').width())/2;
			$('.spinner-box').css({'top':loadingTop,'left':loadingLeft});
			var loadingTop1=($('.spinner-box').height()-$('.spinner').height())/2;
			var loadingLeft1=($('.spinner-box').width()-$('.spinner').width())/2;
			$('.spinner').css({'top':'1.2em','left':loadingLeft1});
		}

        if (type == "tip") {

            var notifybarH = $('.notifybar').height();

            var notifybarW = $('.notifybar').width();

            $('.notifybar').css({
                'top': -(notifybarH + 16),
                'margin-left': -((notifybarW + 60) / 2)
            });

            $('.notifybar').css({
                'display': 'block'
            }).animate({
                'top': 0
            },
            function() {

                setTimeout(function() {
                    $('.notifybar').animate({
                        'top': -(notifybarH + 16)
                    },
                    function() {

                        $(this).css({
                            'display': 'none'
                        }).remove();

                        setTimeout(callback, 1);

                    });
                },
                1500);

            });

        }

        var boxHeight = $("#mb_box").height();

        $('#mb_box').css({
            //'opacity': 1,
            'margin-top': -((boxHeight) / 2),
            //'margin-left':100,
            'transition':'all 0.5s ease',
            '-moz-transition':'all 0.5s ease',
            '-webkit-transition':'all 0.5s ease',
            '-o-transition':'all 0.5s ease'
        });

        oldajaxwinH = $('#mb_box').height();

        oldajaxwinW = $('#mb_box').width();

    }

    //生成Css

    var GenerateCss = function(type) {

        /*



        $("#mb_box").css({ width: '100%', height: '100%', zIndex: '99999', position: 'fixed',



            filter: 'Alpha(opacity=60)', backgroundColor: 'black', top: '0', left: '0', opacity: '0.6'



        });



 



        $("#mb_con").css({ zIndex: '999999', width: '400px', position: 'fixed',



            backgroundColor: 'White', borderRadius: '15px'



        });



 



        $("#mb_tit").css({ display: 'block', fontSize: '14px', color: '#444', padding: '10px 15px',



            backgroundColor: '#DDD', borderRadius: '15px 15px 0 0',



            borderBottom: '3px solid #009BFE', fontWeight: 'bold'



        });



 



        $("#mb_msg").css({ padding: '20px', lineHeight: '20px',



            borderBottom: '1px dashed #DDD', fontSize: '13px'



        });



 



        $("#mb_ico").css({ display: 'block', position: 'absolute', right: '10px', top: '9px',



            border: '1px solid Gray', width: '18px', height: '18px', textAlign: 'center',



            lineHeight: '16px', cursor: 'pointer', borderRadius: '12px', fontFamily: '微软雅黑'



        });



 



        $("#mb_btnbox").css({ margin: '15px 0 10px 0', textAlign: 'center' });



        $("#mb_btn_ok,#mb_btn_no").css({ width: '85px', height: '30px', color: 'white', border: 'none' });



        $("#mb_btn_ok").css({ backgroundColor: '#168bbb' });



        $("#mb_btn_no").css({ backgroundColor: 'gray', marginLeft: '20px' });



 



 



        //右上角关闭按钮hover样式



        $("#mb_ico").hover(function () {



            $(this).css({ backgroundColor: 'Red', color: 'White' });



        }, function () {



            $(this).css({ backgroundColor: '#DDD', color: 'black' });



        });



		*/

        var _widht = document.documentElement.clientWidth; //屏幕宽

        var _height = document.documentElement.clientHeight; //屏幕高

        var boxWidth = $("#mb_box").width();

        var boxHeight = $("#mb_box").height();

        //让提示框居中
        $("#mb_box").css({
           // 'opacity': 0,
            'margin-top': -_height/2,
            'margin-left': -((boxWidth) / 2)
        });
      
		
		
    }

    //确定按钮事件

    var btnOk = function(callback) {

        $("#mb_btn_ok").click(function() {

            $('.bg').addClass('display-none');

            $("#mb_box,#mb_con,#mb_bg").remove();

            if (typeof(callback) == 'function') {

                callback();

            }

        });

    }

    //取消按钮事件

    var btnNo = function() {

        $("#mb_btn_no,#mb_ico,#mb_bg").click(function() {

            $("#mb_box,#mb_con,#mb_bg").remove();

        });

    }

})(); 
