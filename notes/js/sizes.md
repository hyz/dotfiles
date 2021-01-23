
### https://stackoverflow.com/questions/3437786/get-the-size-of-the-screen-current-web-page-and-browser-window

You can get the size of the window or document with jQuery:

    // Size of browser viewport.
    $(window).height();
    $(window).width();

    // Size of HTML document (same as pageHeight/pageWidth in screenshot).
    $(document).height();
    $(document).width();

For screen size you can use the screen object:

    window.screen.height;
    window.screen.width;



https://andylangton.co.uk/blog/development/get-viewportwindow-size-width-and-height-javascript
