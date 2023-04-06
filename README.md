[![Build Status](https://travis-ci.com/watermelon503/bitly.svg?token=uPXx2RMyux8y6zxLL8f6&branch=main)](https://travis-ci.com/watermelon503/bitly)
# WordPress Bitly Integration Plugin #
This plugin is used to integrate WordPress posts with [Bitly](https://bitly.com/) by generating a Bitly shortlink for selected post types. It has been tested up to WordPress version 5.8.
##  Installation ##
Note: you must have a Bitly account in order to use this plugin. Any level of account will work.
* Either install automatically through the WordPress admin, or download the .zip file, unzip to a folder, and upload the folder to your /wp-content/plugins/ directory. Read Installing Plugins in the WordPress Codex for details.
* Activate the plugin through the Plugins menu in WordPress.
## Configuration ##
Settings for this plugin are found on the Settings->Writing page. You can also access this page directly via the Settings link under this plugin on the Plugins page.

To begin using this plugin, you must first obtain an Authorization Token. 
To do so, in a separate browser tab login to the Bitly account to which you would like to connect. 
Back on the WordPress Writing Settings page, simply click the large Authorize button next 
to "Connect with Bitly". If that works successfully the "Bitly OAuth Token" field will be populated with an Authorization Token string.

Once you have an Authorization Token in place, you can proceed with the related configuration settings.
* **Post Types:** Check which available post types will automatically have shortlinks created automatically upon creation. 
* **Default Group:** This select box will allow users with [Enterprise] (https://bitly.com/pages/pricing) level accounts to choose which Group the shortlinks will be associated with. Other account levels will just see their default Group listed.
* **Default Domain:** This select box will allow users with [Basic or Enterprise] (https://bitly.com/pages/pricing) level accounts to choose the shortlink domain that will be used for link creation. By default (and the only option for Free users) this is bit.ly.
* **Debug WP Bitly:** Checking this will create a debug log in /wp-content/plugins/wp-bitly/log/debug.txt.
## Creating Shortlinks ##
There are two ways to create shortlinks:
* **Using the Post Type Configuration Option:** If all posts of a certain post type are to have shortlinks, simply check that post type's box in the Settings page to automatically create shortlinks for that post type on publish.
* **Using the Supplied Shortcode:** If a post type is not checked but a shortlink is to be created for a particular post, simply add the shortcode [wpbitly] in the post content. Upon publish a shortlink will be created. Alternatively, click on the red "Generate new Shortlink" link within the WP Bitly meta box on any published item without an existing shortlink.
Regardless of what method is chosen, the created shortlink can be accessed in the WP Bitly section of the post in the main post attributes part of the screen. Clicking on the "Get Shortlink" button will create a popup of the created shortlink. In addition, statistics relating to the shortlink will appear to include the number of clicks today, the total number clicks over time, and a graph of the number of clicks over the last 7 days.

The screenshot below shows the meta box created for a post that has an associated shortlink. The post edit page may need to be refreshed to see it.

![bityly_metabox](https://user-images.githubusercontent.com/1296721/102672953-29850900-4147-11eb-92ce-2133241ab94b.jpg)
