
### https://github.com/FormidableLabs/spectacle

    create-react-app my-presentation --scripts-version spectacle-scripts
    cd my-presentation
    yarn run build
    miniserve build

### https://github.com/jaspervdj/patat

Terminal-based presentations using Pandoc


### https://github.com/isnowfy/pydown

pydown is another "Presentation System in a single HTML page" written by python inspired by keydown.
Like keydown it uses the deck.js and its extentions for the presentation.

### https://github.com/hakimel/reveal.js
### https://github.com/webpro/reveal-md

    reveal-md -p 8000 -h 0 oo.md --static ooslide

### http://mindfulstart.net/slides.html

    pandoc oo.md -o oo.html -t revealjs -s -V theme=white

### https://github.com/jgm/pandoc/issues/2157

    title: test reveal.js slideshow
    author: author’s name
    date: May 18, 2015

    # 1st Header Slide
    ## 1st Slide Level Slide
    level 2 is the “slide level”. Because the 1st Header Slide has no content, this slide is nested.

