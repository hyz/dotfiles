
    > docto -XL -f .\2204_A3-1_意城家品.xls -O out-xlsx -T xlWorkbookDefault

    > sudo pacman -Sy libreoffice-fresh
    > libreoffice --headless --convert-to xlsx my.xls # pacman -S libreoffice-fresh

### https://github.com/SheetJS/js-xlsx

Browser file upload form element
Data from file input elements can be processed using the same FileReader API as in the drag-and-drop example:

    var rABS = true; // true: readAsBinaryString ; false: readAsArrayBuffer
    function handleFile(e) {
      var files = e.target.files, f = files[0];
      var reader = new FileReader();
      reader.onload = function(e) {
        var data = e.target.result;
        if(!rABS) data = new Uint8Array(data);
        var workbook = XLSX.read(data, {type: rABS ? 'binary' : 'array'});

        /* DO SOMETHING WITH workbook HERE */
      };
      if(rABS) reader.readAsBinaryString(f); else reader.readAsArrayBuffer(f);
    }
    input_dom_element.addEventListener('change', handleFile, false);

### c++

    http://www.cnblogs.com/fullsail/archive/2012/12/28/2837952.html
    http://blog.csdn.net/fullsail/article/details/4067416

### http://xmodulo.com/how-to-convert-xlsx-files-to-xls-or-csv.html
### http://xmodulo.com/how-to-convert-xlsx-files-to-xls-or-csv.html

### https://sourceforge.net/projects/pyexcelerator/

### https://stackoverflow.com/questions/9918646/how-to-convert-xls-to-xlsx

### https://stackoverflow.com/questions/151005/create-excel-xls-and-xlsx-file-from-c-sharp





