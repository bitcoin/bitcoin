=== Bitly's Wordpress Plugin ===
Contributors: clint.s, Kelseystevensonbitly
Donate link: https://watermelonwebworks.com
Tags: shortlink, bitly, url, shortener, custom domain, social, media, twitter, facebook, share
Requires at least: 5.0
Tested up to: 6.0.1
Stable tag: 2.7.1
License: GPLv2 or later
License URI: http://www.gnu.org/licenses/gpl-2.0.html

Create short links to your content with Bitly’s WordPress Plugin.

== Description ==

Love WordPress? Love Bitly? After installing this plugin, you’ll be able to shorten a link and view clicks right from WordPress. Your new links will be saved to Bitly for reference and deeper analysis.

*To do that, you must have a Bitly account to use the plugin. Your account is where you store, edit, and view metrics for your links. Register at bitly.com.*

No matter the type of site you own (from a personal blog to an ecommerce store and everything in between) Bitly makes it easy to create shorter links and keep an eye on your clicks. Whether you share your links on social, SMS, or email, a short link is easier to manage and remember.  

== Installation ==
# Installing the plugin

You must have a Bitly account to connect to. You can create one at bitly.com. Your account is where you store, edit, and view metrics for your links.
If you're familiar with adding a plugin to WordPress directly, you can download the ZIP file and upload it to /wp-content/plugins/ after step 2. Otherwise, follow these steps:
1. Go to your WordPress plugins settings.
2. Click Add New.
3. Search for Bitly's WordPress Plugin.
4. Click Install Now.
5. Click Activate.
6. Scroll down the list of your installed plugins, find 
Bitly, and click Settings.
7. Now you'll connect the plugin to your Bitly account. Scroll down to Bitly's WordPress Plugin and click Authorize.
8. Sign in to Bitly, if necessary, and click Allow. The plugin will retrieve an authorization from Bitly and update the Bitly OAuth Token field.
9. Scroll down and click Save Changes.
10. Select which types of entries you'll be creating short links for.
11. If your Bitly account has more than one group, select a group. This is the group where your short links will be saved in Bitly.
12. If your Bitly account has more than one custom domain, select a domain. This is the domain that will be used for your short links.
13. Click Save Changes.

Now you can shorten links when you create a new post!

# Creating a short link

After publishing a page or post, you’ll see the Bitly's Wordpress Plugin option in the settings. Click Get Shortlink to create a link to the current content. The link will be saved in your Bitly account, and you’ll be able to view the number of clicks it receives right from this setting.

# Viewing a link’s click data

Click metrics for a short link will appear in the post’s or page’s settings. They include the number of clicks today, the total number clicks over time, and a graph of the number of clicks over the last 7 days.

== Frequently Asked Questions ==

= After installation, do I need to update all my posts for short links to be created? =

No. The first time a short link is requested for a particular post, the plugin will automatically generate one.

== Changelog ==
= 2.7.1 =
* Updated to prevent short link generation when a post is viewed on the fronted, or edited (but not re-saved) on the backend.
= 2.7.0 =
* Updated to prevent short link (re)generation when bulk-editing posts.
* Added placeholder message to Bitly metabox when creating new post.
* Fixed issue where "Default Organization" field displayed incorrect value after configuration change.
* Various additional minor fixes.
= 2.6.0 =
* Completely rebuilt for use with Bitly API version 4.
= 2.5.2 =
* Fixes various php warnings produced by assuming $post
* Better response handling for wpbitly_get()
= 2.5.0 =
* Adds "Regenerate Shortlink" feature to pages and posts
* Adds chart showing previous 7 days of activity
= 2.4.3 =
* Adds debugging to authorization process
* Adds manual entry of the OAuth token in case automatic authorization fails
= 2.4.1 =
* Backwards compatible with PHP 5.2.4
* Updates styling on settings page
* Updates `wp_generate_shortlink()`
* Adds debug setting back
= 2.4.0 =
* Updated for use with WordPress 4.9.2 and current Bitly Oauth
= 2.3.2 =
* Fixed a typo in `wpbitly_shortlink`
= 2.3.0 =
* Trimmed excess bloat from `wp_get_shortlink()`
* Tightened up error checking in `wp_generate_shortlink()`
= 2.2.6 =
* Fixed bug where shortlinks were generated for any post type regardless of setting
* Added `save_post` back, further testing needed
= 2.2.5 =
* Added the ability to turn debugging on or off, to aid in tracking down issues
* Updated to WordPress coding standards
* Removed `wpbitly_generate_shortlink()` from `save_post`, as filtering `pre_get_shortlink` seems to be adequate
* Temporarily removed admin bar shortlink button (sorry! it's quirky)
= 2.2.3 =
* Replaced internal use of cURL with wp_remote_get
* Fixed a bug where the OAuth token wouldn't update
= 2.0 =
* Updated for WordPress 3.8.1
* Updated Bitly API to V3
= 1.0.1 =
* Fixed bad settings page link in plugin row meta on Manage Plugins page
= 1.0 =
* Updated for WordPress 3.5
* Removed all support for legacy backwards compatibility
* Updated Settings API implementation
* Moved settings from custom settings page to Settings -> Writing
* Enabled shortlink generation for scheduled (future) posts
* Added I18n support.
= 0.2.6 =
* Added support for automatic generation of shortlinks when posts are viewed.
= 0.2.5 =
* Added support for WordPress 3.0 shortlink API
* Added support for custom post types.
= 0.1.0 =
* Initial release of WP Bitly
