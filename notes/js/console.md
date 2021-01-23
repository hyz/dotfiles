
### https://stackoverflow.com/questions/7474354/include-jquery-in-the-javascript-console

Run this in your browser's JavaScript console, then jQuery should be available...

    var jq = document.createElement('script');
    jq.src = "https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js";
    document.getElementsByTagName('head')[0].appendChild(jq);
    // ... give time for script to load, then type (or see below for non wait option)
    jQuery.noConflict();

NOTE: if the site has scripts that conflict with jQuery (other libs, etc.) you could still run into problems.
Update:

Making the best better, creating a Bookmark makes it really convenient, let's do it, and a little feedback is great too:

    Right click the Bookmarks Bar, and click Add Page
    Name it as you like, e.g. Inject jQuery, and use the following line for URL:

    javascript:(function(e,s){e.src=s;e.onload=function(){jQuery.noConflict();console.log('jQuery injected')};document.head.appendChild(e);})(document.createElement('script'),'//code.jquery.com/jquery-latest.min.js')

Below is the formatted code:

    javascript: (function(e, s) {
        e.src = s;
        e.onload = function() {
            jQuery.noConflict();
            console.log('jQuery injected');
        };
        document.head.appendChild(e);
    })(document.createElement('script'), '//code.jquery.com/jquery-latest.min.js')

Here the official jQuery CDN URL is used, feel free to use your own CDN/version.
Share
Improve this answer
Follow
edited Aug 18 '20 at 18:47
Geremia
2,5222525 silver badges3030 bronze badges
answered Sep 19 '11 at 16:44
jondavidjohn
58.6k2121 gold badges110110 silver badges154154 bronze badges

    191
    This snippet didn't work for me. Had no time to figure it out why. Just copied 

        http://code.jquery.com/jquery-latest.min.js

    file content and pasted into console. Works perfect. – Ruslanas Balčiūnas Nov 22 '12 at 11:32


