function MinuteLine(elm) {

	var obj = this;
	this.new_price = 0;
	this.data = {}; //分时线数据
	this.line_num=0;
	this.dot=2;
	this.elm = elm; //图像显示的Div
	this.mouse = {
		x: 0,
		y: 0
	}; //保存鼠标坐标
	this.proc = false;
	this.priceMaxMin = {
		maxnum: 0,
		minnum: 0
	};
	this.VolMaxMin = {
		maxnum: 0,
		minnum: 0
	};
	this.rect = {
		x: 0,
		y: 0,
		height: ($(this.elm).height()),
		width: $(this.elm).width()
	}; //整个画图区域
	this.line_rect = {
		x: 0,
		y: 0,
		width: (this.rect.width),
		height: (this.rect.height / 3 * 2)
	} ///分时线画图区域
	this.init();
};
MinuteLine.prototype.chageinit = function() {
	this.rect = {
		x: 0,
		y: 0,
		height:$(this.elm).height(),
		width: $(this.elm).width()
	}; //整个画图区域
	this.line_rect = {
		x: 0,
		y: 0,
		width: this.rect.width,
		height: this.rect.height - 20
	} ///分时线画图区域
	this.volrect = {
		x: 0,
		y: (this.rect.height / 3 * 2),
		width: (this.rect.width),
		height: (this.rect.height / 3) - 20
	}; ///成交量区域
	var num = this.getPixelRatio(this.ctx);
    
	$('#Canvas').attr("height", this.rect.height * num);
	$('#Canvas').attr("width", this.rect.width * num);
	this.ctx.clearRect(0, 0, this.rect.width * num, this.rect.height * num);
	$('#Canvas').css({
		"width": this.rect.width,
		"height": this.rect.height
	});


};
MinuteLine.prototype.getPixelRatio = function(context) {
	var backingStore = context.backingStorePixelRatio || context.webkitBackingStorePixelRatio || context.mozBackingStorePixelRatio || context.msBackingStorePixelRatio || context.oBackingStorePixelRatio || context.backingStorePixelRatio || 1;
	return (window.devicePixelRatio || 1) / backingStore;
};
MinuteLine.prototype.init = function() { //初始化
	//this.chageinit();
	$('#Canvas').remove();
	var htmls = '<canvas id="Canvas" width="' + this.rect.width + 'px" height="' + $(this.elm).height() + 'px"></canvas>';
	$(this.elm).html(htmls);
	//$('#Canvas').css({"display": "block"});
	this.c = document.getElementById("Canvas");
	this.ctx = this.c.getContext("2d");
	this.ctx.translate(0.5, 0.5);
	$('#Canvas').css({
		"width": this.rect.width,
		"height": this.rect.height
	});
	this.ctx.lineWidth = 1;

	$(window).resize(function() {

		this.chageinit();
		this.drawimg();

	});
	document.getElementById(this.elm.replace("#", '')).addEventListener('touchmove',
	function(event) {
		event.preventDefault();
	},
	false);
	this.bindEvent();

};
MinuteLine.prototype.bindEvent = function() {
	/* $(elm).bind("touchstart", this.touchSatrtFunc);
			    $(elm).bind("touchmove", this.touchMoveFunc);
				 $(elm).bind("touchend", this.touchEndFunc);*/
	var obj = this;
	/*document.getElementById(this.elm.replace("#", '')).addEventListener('touchstart',function(evt){
			
			
			}, false);*/
	document.getElementById(this.elm.replace("#", '')).addEventListener('touchmove',
	function(evt) {

		evt.preventDefault(); //阻止触摸时浏览器的缩放、滚动条滚动等
		var touch = evt.touches[0]; //获取第一个触点
		var x = Number(touch.pageX); //页面触点X坐标
		//alert($(elm).pageY);
		var y = Number(touch.pageY) - $(obj.elm).height() / 2; //页面触点Y坐标
		obj.mouse.x = x;
		obj.mouse.y = y;
		obj.ctx.clearRect(0, 0, obj.rect.width, obj.rect.height);
		obj.proc = true;
		obj.drawimg();

	},
	false);
	document.getElementById(this.elm.replace("#", '')).addEventListener('touchend',
	function(evt) {
		obj.ctx.beginPath();
		obj.ctx.clearRect(0, 0, obj.rect.width, obj.rect.height);
		obj.proc = false;
		obj.drawimg();

	},
	false);

};

MinuteLine.prototype.drawLine = function() { //画分时线

	var kd,kd2;
	var scal = this.line_rect.height / (this.priceMaxMin.maxnum - this.priceMaxMin.minnum);
	
	var ystep = (this.priceMaxMin.maxnum - this.priceMaxMin.minnum) / 6;
	var gitasx = 0;
	this.ctx.beginPath();
	this.ctx.strokeStyle = "#79161b";
	this.ctx.fillStyle = "#ff0000";
	this.ctx.font = "normal normal lighter 1em arial";
	var i=0;
	 if(this.priceMaxMin.minnum==0||this.priceMaxMin.maxnum==0){return;};
	while ((gitasx + this.priceMaxMin.minnum) <= this.priceMaxMin.maxnum) {
  i++;
		this.ctx.moveTo(0, this.line_rect.height - (gitasx * scal));
		 
		this.ctx.lineTo(this.line_rect.width, this.line_rect.height - (gitasx * scal));
         kd2 = this.priceMaxMin.minnum + gitasx;
		
		 this.ctx.fillText(parseFloat(kd2).toFixed(this.dot), 0, this.line_rect.height - (gitasx * scal) - 2);
		gitasx+= ystep;
		
	}
	this.ctx.stroke();
	this.ctx.beginPath();
	this.ctx.strokeStyle = "#ffffff";
	var last_line = {
		x: 0,
		y: 0
	};
	var last_price;
	this.ctx.moveTo(0, this.line_rect.height - (this.data[0].price - this.priceMaxMin.minnum) * scal);
	for (i = 0; i < this.data.length; i++) {
		kd = this.line_rect.height - (this.data[i].price - this.priceMaxMin.minnum) * scal;
		this.ctx.lineTo(this.line_rect.width / this.line_num * i, kd);
		if (i == this.data.length - 1) {
			last_line.x = this.line_rect.width /  this.line_num * i;
			last_line.y = kd;
			last_price = this.data[i].price;
		}

	}
	this.ctx.stroke();
	this.ctx.beginPath();
	this.ctx.strokeStyle = "#efba02";
	if (this.new_price > 0) {
		this.new_price = this.new_price;
	} else {
		this.new_price = last_price;
	};
	kd = this.line_rect.height - (this.new_price - this.priceMaxMin.minnum) * scal;
	this.ctx.moveTo(0, kd);
	this.ctx.lineTo(this.line_rect.width, kd);
	this.ctx.stroke();
	this.ctx.beginPath();
	this.ctx.arc(last_line.x, kd, 5, 0, Math.PI * 2, true);
	this.ctx.closePath();
	this.ctx.fillStyle = '#efba02';
	this.ctx.font = "normal normal lighter 2em arial";
	this.ctx.fill();
	var metrics =this.ctx.measureText(this.new_price); 
	
	if(kd>=this.line_rect.height){
		kd=this.line_rect.height-30;
		};
	if((kd-10)<=20){
		
		kd=50;
		};
	if((last_line.x + 10+metrics.width)>this.line_rect.width){
		last_line.x=this.line_rect.width-20;
		}
	if((last_line.x+100)>=this.line_rect.width){
	//this.ctx.fillText(this.new_price, last_line.x -100, kd +20);
	}else{
	//this.ctx.fillText(this.new_price, last_line.x +10, kd - 20);
	}
	

};
MinuteLine.prototype.drawtip = function() {

	i = Math.ceil(this.mouse.x / (this.rect.width /  this.line_num));
	if (i > ( this.line_num-1) || i > this.data.length) {
		return;
	};
	if (this.proc == true) {
		this.ctx.beginPath();
		this.ctx.fillStyle = "#000000";
		var x;
		if (this.mouse.x > 100) {
			x = 10;
			this.ctx.fillRect(0, 10, 80, 130);
		} else {
			this.ctx.fillRect(this.rect.width - 80, 10, 80, 130);
			x = this.rect.width - 70;
		}
		var d = new Date(parseInt(this.data[i].time) * 1000);
		this.ctx.stroke();
		this.ctx.beginPath();
		this.ctx.strokeStyle = "#ffffff";
		this.ctx.fillStyle = "#ffffff";
		this.ctx.font = "normal normal lighter 1em arial";
		this.ctx.strokeRect(x - 10, 10, 81, 130);
		this.ctx.fillText("价格", x, 35);
		this.ctx.fillText(this.data[i].price, x, 65);

		this.ctx.fillText("时间", x, 95);
		var m=d.getMinutes();
		var h=d.getHours();
		if(m<10){
		  m="0"+m;
		}
		
		if(h<10){
		  h="0"+h;
		}
		this.ctx.fillText(h+ ":" +m, x, 125);

		this.ctx.stroke();

	}
};
MinuteLine.prototype.drawimg = function() {

	this.ctx.strokeStyle = "#79161b";
	this.ctx.fillStyle = "#ffffff";
	this.ctx.font = "normal normal lighter 1em arial";
	for (i = 0; i <= 4; i++) {
		if (i == 0) {
			//this.ctx.fillText("08:00", 0, this.rect.height - 5);
		}
		if (i == 1) {
			//this.ctx.fillText("13:30", this.rect.width / 4 * i - 20, this.rect.height - 5);
		}
		if (i == 2) {
			//this.ctx.fillText("18:30", this.rect.width / 4 * i - 20, this.rect.height - 5);
		}
		if (i == 3) {
			//this.ctx.fillText("23:30", this.rect.width / 4 * i - 20, this.rect.height - 5);
		}
		if (i == 4) {
			//this.ctx.fillText("06:00", this.rect.width - 40, this.rect.height - 5);
		}
		this.ctx.moveTo(Math.ceil(this.rect.width / 4 * i), 0);
		this.ctx.lineTo(Math.ceil(this.rect.width / 4 * i), this.rect.height - 20);

	}

	this.ctx.stroke();

	this.drawLine();
	this.ctx.save();

	this.ctx.restore();
	this.ctx.beginPath();
	if (this.proc == true) {

		this.ctx.strokeStyle = "#e9e8e7";
		this.ctx.moveTo(this.mouse.x, 0);
		this.ctx.lineTo(this.mouse.x, this.rect.height - 20);
		this.ctx.moveTo(0, this.mouse.y);
		this.ctx.lineTo(this.rect.width, this.mouse.y);
		this.ctx.stroke();

	}
	this.drawtip();

};

MinuteLine.prototype.get_max_min = function(price) {

	var maxnum = 0,minnum = 0;
	for (var i = 0; i < this.data.length; i++) {

		if (i == 0) {
			var numi = parseFloat(this.data[i].price);
			maxnum = numi;
			minnum = numi

		} else {
			var numi = parseFloat(this.data[i].price);
			numi > maxnum ? maxnum = numi: '';
			numi < minnum ? minnum = numi: ''

		}

	};
	if (this.new_price > 0) {
		if (this.new_price > maxnum) {
			maxnum = this.new_price;
		};
		if (this.new_price < minnum) {
			minnum = this.new_price;
		};
	}
	this.priceMaxMin = {
		maxnum: maxnum,
		minnum: minnum

	}

};
MinuteLine.prototype.setData = function(data,num,time) {
	this.data = data;
    this.new_price=0;
	this.get_max_min(true);
    this.line_num=time
	if(num==4){
	 this.dot=5;
	}else if(num==3){
	this.dot=3;
	}else{
	this.dot=2;
	}
	this.chageinit();
	this.drawimg();

};
MinuteLine.prototype.Update = function(data) {

	this.data.push(data);
	
	this.get_max_min(true);
	
	this.chageinit();
	this.drawimg();
};
MinuteLine.prototype.UpdatePrice = function(price) {
	if (this.data.length <= 0) {
		return;
	};

	this.new_price = price;
	this.get_max_min(true);
	this.chageinit();

	this.drawimg();

};