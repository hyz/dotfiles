Date.prototype.GetTime = function(time) {
    var d = new Date(parseInt(time) * 1000);
    var m = "";
    if (d.getHours().length == 1) {
        m = "0" + d.getHours();
    } else {
        m = d.getHours();

    }
    return m + ":" + d.getMinutes();

};
Date.prototype.timer=function(elm){
	var obj=this;
	window.setInterval(function(){
	  obj.Daojishi(elm);
	}, 1000);
} 
Date.prototype.Daojishi=function(elm){
	
	$(elm).each(function(){
		  var datastr = $(this).attr('time'); 
		 var  newstr=datastr.replace(/-/g,'/')
		var intDiff=Date.parse(new Date(newstr))/1000;
		 intDiff=intDiff-Date.parse(new Date())/1000;
		 console.log(intDiff);
		var day=0,
		hour=0,
		minute=0,
		second=0;//时间默认值		
	if(intDiff > 0){
		day = Math.floor(intDiff / (60 * 60 * 24));
		hour = Math.floor(intDiff / (60 * 60)) - (day * 24);
		minute = Math.floor(intDiff / 60) - (day * 24 * 60) - (hour * 60);
		second = Math.floor(intDiff) - (day * 24 * 60 * 60) - (hour * 60 * 60) - (minute * 60);
	}
	if (minute <= 9) minute = '0' + minute;
	if (second <= 9) second = '0' + second;
	var str=minute+second;
	if(str=="0000"){
		$(this).parent("li").parent('.pro_list').remove();
		}
	$(this).html("结算:"+minute+"分"+second+"秒");

	 
		});

	
	
	};
Date.prototype.setTimeList = function(data, callback) {
    this.time = data;
    var list = [];
    for (i = 0; i < this.time.length; i++) {
        list.push(this.GetTime(this.time[i]));
    };
    callback(list);
	var obj=this;
    this.timechek = setInterval(function() {
        for (i = 0; i < obj.time.length; i++) {
            var t = Date.parse(new Date()) / 1000;
            if (obj.time[i] < t) {

                obj.time[i].shift();
                list = [];
                for (j = 0; j < obj.time.length; j++) {
                    list.push(obj.GetTime(obj.time[j]));
                };
                callback(list);
                break;
            }
            break;

        };
		console.log("shuaxin");
    },
    1000 * 60);

};
Date.prototype.clearTime = function() {
    clearInterval(this.timechek);
};