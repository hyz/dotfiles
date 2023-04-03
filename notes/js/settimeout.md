
    var lis = document.querySelectorAll('li.list-view__item > a:nth-child(1)') //li.list-view__item:nth-child(6) > a:nth-child(1) 
    function fx(len) { if (len >= 0) setTimeout(()=> {  console.log(len);  fx(len-1); }, 5000); }
    fx(lis.length)

