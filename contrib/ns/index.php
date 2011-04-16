<?php
require 'util.php';
require 'header.php';

function add_nickname()
{
    $uid = user_id();
    $nickname = post('nickname');
    $query = "
        SELECT
            1
        FROM
            nicknames
        WHERE
            nickname='$nickname'
            ";
    $result = do_query($query);
    if (has_results($result)) 
        throw new Problem('Cannot', 'This nickname is already taken by another person.');

    $query = "
        INSERT IGNORE INTO
            nicknames(
                nickname,
                uid
            )
        VALUES (
            '$nickname',
            '$uid'
        )
        ";
    do_query($query);
}

function remove_nickname()
{
    $uid = user_id();
    $nickname = post('nickname');
    $query = "
        DELETE FROM
            nicknames
        WHERE
            uid='$uid'
            AND nickname='$nickname'
        ";
    do_query($query);
}

function update_pubkey()
{
    $uid = user_id();
    $pubkey = post('pubkey');
    $query = "
        UPDATE
            users
        SET
            pubkey='$pubkey'
        WHERE
            uid='$uid'
        ";
    do_query($query);
    echo "<p><b>Updated public key.</b></p>";
}

function perform_action()
{
    $action = $_POST['action'];
    switch ($action) {
        case 'add_nickname':
            add_nickname();
            break;

        case 'remove_nickname':
            remove_nickname();
            break;

        case 'update_pubkey':
            update_pubkey();
            break;

        default:
            throw new Error('Unknown action', 'This action is unknown');
            break;
    }
}

function display_info()
{
    $uid = user_id();
    $query = "
        SELECT
            uid,
            oidlogin,
            pubkey,
            timest
        FROM
            users
        WHERE
            uid='$uid'
            ";
    $result = do_query($query);
    $row = get_row($result);
    $oidlogin = $row['oidlogin'];
    if (isset($row['pubkey']))
        $pubkey = $row['pubkey'];
    else
        $pubkey = '';
    $timest = $row['timest'];
?>
    <ul>
        <li><?php echo $oidlogin; ?></li>
        <li><?php echo $timest; ?></li>
    </ul>
    <form action='' method='post'>
        <label for='pubkey'>Public key:</label>
        <textarea name='pubkey'><?php echo $pubkey; ?></textarea>
        <input type='hidden' name='action' value='update_pubkey' />
        <input type='submit' value='Update' />
    </form>
<?php

    $query = "
        SELECT
            nickid,
            nickname,
            addr,
            uid,
            timest
        FROM
            nicknames
        WHERE
            uid='$uid'
        ";
    $result = do_query($query);
?>
<br />
<table id='prop'>
    <tr>
        <th>Nickname</th>
        <th>Address</th>
        <th></th>
    </tr>
<?php
    while ($row = mysql_fetch_assoc($result)) {
        $nickname = $row['nickname'];
        echo "    <tr>\n";
?>
    <td><?php echo $nickname; ?></td>
    <td><?php
        if (isset($row['addr']))
            echo $row['addr'];
        else
            echo 'None set';
    ?></td>
    <td>
        <form action='' method='post'>
            <input type='hidden' name='action' value='remove_nickname' />
            <input type='hidden' name='nickname' value='<?php echo $nickname; ?>' />
            <input type='submit' value='Remove' />
        </form>
    </td>
<?php
        echo "    </tr>\n";
    }
    echo "</table>\n";
}

function main_page()
{
    if (isset($_SESSION['uid'])) {
        write_header();
        if (isset($_POST['action'])) {
            perform_action();
        }

        display_info();
    ?>
    <br />
    <form action='' method='post'>
        <label for='nickname'>Nickname:</label>
        <input type='text' name='nickname' />
        <input type='hidden' name='action' value='add_nickname' />
        <input type='submit' value='Add nickname' />
    </form>
    <?php
    }
    else {
        header('Location: login.php');
    }
}

try {
    main_page();
}
catch (Error $e) {
    echo "<div class='content_box'><h3>{$e->getTitle()}</h3>";
    echo "<p>{$e->getMessage()}</p></div>";
    echo "<p><a href=''>Back to main page</a></p>";
}
catch (Problem $e) {
    echo "<div class='content_box'><h3>{$e->getTitle()}</h3>";
    echo "<p>{$e->getMessage()}</p></div>";
    echo "<p><a href=''>Back to main page</a></p>";
}

write_footer();

