#!/bin/sh

# Make index.html of all man page images so we can see them in Evergreen.

echo "<html>
  <head>
    <title>Man Page Images</title>
    <style type="text/css">
      img { border: 1px solid darkred; }
      p.img-name { margin-left: 5em; }
    </style>
  </head>
  <body>"

for path in $1/*.png; do
    png=$(basename $path)
    case "$png" in
    # These pages include all others, CDRIVER-2035
    bson_api.3.png)
        continue
    ;;
    bson_index.3.png)
        continue
    ;;
    *)
        echo "
  <p class="img-name"><a href=\"${png}\">${png}</a></p>
  <p><a href=\"${png}\"><img src=\"${png}\"></a></p>"
        ;;
    esac
done

echo "</body></html>"
