/*! afscript.js | Copyright (c) 2016 Pdfix. All Rights Reserved. |  GNU GPLv3 license */
function fix_page_size() { if ("fixed" == document.getElementById("pdf-document").getAttribute("layout")) for (var b = 0; ;) { var a = document.getElementById("page-" + b++); if (void 0 == a) break; a.style.height = a.getAttribute("r") * a.offsetWidth + "px" } }
function fix_text_objects(){$('span[name="fix-text"]').each(function(){var b=parseFloat($(this).outerWidth()),a=$(this).attr("ow");void 0==a?$(this).attr("ow",b):b=parseFloat(a);var a=parseFloat($(this).parent().outerWidth()),b=parseFloat(a/b),a=parseFloat($(this).outerHeight()),c=parseFloat($(this).parent().outerHeight()),a=parseFloat(c/a);$(this).css("transform","scaleX("+b+") scaleY("+a+")");$(this).css("transform-origin","0px 0px 0px");$(this).css("display","inherit")})};
