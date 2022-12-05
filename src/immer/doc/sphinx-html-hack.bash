#!/bin/bash

location=`dirname $0`

echo "Running $0 at $location"

# Fixes issues described here among others
# https://github.com/michaeljones/breathe/issues/284

fix-missing-class-name()
{
    src='<span id="\([^"]*\)"></span>\(.*\)<em class="property">class </em>'
    dst='<span id=""></span>\2<em class="property">class</em><tt class="descname">\1</tt>'
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}

fix-missing-struct-name()
{
    src='<span id="\([^"]*\)"></span>\(.*\)<em class="property">struct </em>'
    dst='<span id=""></span>\2<em class="property">struct</em><tt class="descname">\1</tt>'
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}

fix-double-using-keyword()
{
    src='<em class="property">using</em><em class="property">using </em>'
    dst='<em class="property">using </em>'
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}

fix-do-not-repeat-type-in-member-using-declaration()
{
    src='<em class="property">using </em><code class="descname">\(\([^:]*::\)*\)\([^ ]*\) = \([^<]*\)</code>'
    dst='<em class="property">using </em><code class="descname">\3 = \4</code>'
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}
fix-do-not-repeat-type-in-member-using-declaration

fix-remove-double-class-name()
{
    # src='<code class="descclassname">\([^&]*\)&lt;\([^&]*\)&gt;::</code>'
    # dst='<code class="descclassname">\1::</code>'
    src='<code class="descclassname">\([^<]*\)</code>'
    dst=''
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}

fix-remove-straneous-typedefs()
{
    src='typedef '
    dst=''
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}

fix-remove-straneous-typedefs-2()
{
    src='= typedef '
    dst='= '
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}
fix-remove-straneous-typedefs-2

fix-remove-straneous-using-declarations()
{
    src='<em class="property">using </em>template&lt;&gt;<br />'
    dst=''
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}

fix-remove-straneous-template-in-using-declarations-1()
{
    src='\(<dl class="type">\n<dt[^>]*>\)\ntemplate&lt;&gt;<br />'
    dst='\1'
    pre=':a;N;$!ba;'
    sed -i "$pre;s@$src@$dst@g" $location/_build/html/*.html
}
fix-remove-straneous-template-in-using-declarations-1

fix-remove-straneous-template-in-using-declarations-2()
{
    src='></span>template&lt;&gt;<br /><span '
    dst='></span><span '
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}
fix-remove-straneous-template-in-using-declarations-2

fix-remove-countainer-css-class-in-member-definitions-causing-overflow()
{
    src='breathe-sectiondef\([[:alnum:] _-]*\)container'
    dst='breathe-sectiondef'
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}
fix-remove-countainer-css-class-in-member-definitions-causing-overflow

fix-remove-inherits-from()
{
    src='<p>Inherits from [^/]*</p>'
    dst=''
    sed -i "s@$src@$dst@g" $location/_build/html/*.html
}
fix-remove-inherits-from
