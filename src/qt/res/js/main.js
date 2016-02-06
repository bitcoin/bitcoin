// Gameunits HTML5 main.js

$("[href='#qrcode-modal']").leanModal({top : 10, overlay : 0.5, closeButton: "#qrcode-modal .modal_close"});
$("#start-conversation").leanModal({top : 200, overlay : 0.5, closeButton: "#new-contact-modal .modal_close"});

var qrcode = new QRCode("qrcode", {colorDark:'#0cb3db', colorLight: '#ffffff', correctLevel: QRCode.CorrectLevel.H, width: 220, height: 220,});

function showQRCode(address, label) {

    if(address!=undefined)
        $("#qraddress").val(address);

    if(label!=undefined)
        $("#qrlabel").val(label);

    qrcode.clear();

    var textarea = $("#qrcode-data"),
        data = "gameunits:";

    data += $("#qraddress").val()
          + "?label="     + $("#qrlabel").val()
          + "&narration=" + $("#qrnarration").val()
          + "&amount=" + unit.parse($("#qramount").val(), $("#qrunit").val());

    textarea.text(data);

    qrcode.makeCode(data);
}

var breakpoint = 906, // 56.625em
    footer = $("#footer");

function resizeFooter() {
    $(".ui-buttons").each(function() {
        $(this).css("margin-top", Math.max($("body").height() - ($(this).offset().top + $(this).height()) -65 + parseInt($(this).css("margin-top")), 10) + "px");
    });

    footer.height(Math.max($("body").height() - footer.offset().top, 35) + "px");
};

function updateValue(element) {
    var curhtml = element.html(),
        value   = (element.parent("td").data("label") != undefined ? element.parent("td").data("label") :
                  (element.parent("td").data("value") != undefined ? element.parent("td").data("value") :
                  (element             .data("label") != undefined ? element             .data("label") :
                  (element             .data("value") != undefined ? element             .data("value") : element.text()))));

    var address = element.parents(".selected").find(".address");

    address = address.data("value") ? address.data("value") : address.text();

    element.html('<input class="newval" type="text" onchange="bridge.updateAddressLabel(\'' + address + '\', this.value);" value="' + value + '" size=60 />');

    $(".newval").focus();
    $(".newval").on("contextmenu", function(e) {
        e.stopPropagation();
    });
    $(".newval").keyup(function (event) {
        if (event.keyCode == 13)
            element.html(curhtml.replace(value, $(".newval").val().trim()))
    });

    $(document).one('click', function () {
        element.html(curhtml.replace(value, $(".newval").val().trim()));
    });
}

$(function() {
    // Menu icon onclick
    $("#navlink").on("click", function() {
        $("#layout").toggleClass('active');
    });

    $("#qramount").on("keydown", unit.keydown).on("paste", unit.paste);


    $('.footable').footable({breakpoints:{phone:480, tablet:700}, delay: 50})
    .on({'footable_breakpoint': function() {
            //$('table').trigger('footable_expand_first_row'); uncomment if we want the first row to auto-expand
        },
        'footable_redrawn':  resizeFooter,
        'footable_resized':  resizeFooter,
        'footable_filtered': resizeFooter,
        'footable_paging':   resizeFooter,
        'footable_row_expanded': function(event) {
        var editable = $(this).find(".editable");

        editable.off("dblclick");
        editable.on("dblclick", function (event) {
           event.stopPropagation();
           updateValue($(this));
        }).attr("title", "Double click to edit").on('mouseenter', tooltip);
    }});

    $(".editable").on("dblclick", function (event) {
       event.stopPropagation();
       updateValue($(this));
    }).attr("title", "Double click to edit %column%");

    $(document).ready(function() {
        resizeFooter();
        $("#navitems a[href='#overview']").trigger('click');
    });

    //$('img,i').click(function(e){e.stopPropagation()});

    // On resize, close menu when it gets to the breakpoint
    window.onresize = function (event) {
        if (window.innerWidth > breakpoint)
            $("#layout").removeClass('active');

        resizeFooter();
    };

    // Change page handler
    $("#navitems a").on("click", changePage);

    if(bridge)
        $("[href='#about']").on("click", function() {bridge.userAction(['aboutClicked'])});

    overviewPage.init();
    sendPageInit();
    receivePageInit();
    transactionPageInit();
    addressBookInit();
    ChatInit();
    chainDataPage.init();
    blockExplorerPage.init();

    // Tooltip
    $('[title]').on('mouseenter', tooltip);

    $(".footable tr").on('click', function() {
        $(this).addClass("selected").siblings("tr").removeClass("selected");
    });
});

var prevPage = null;

function changePage(event) {
            var toPage = $($(this).attr('href'));

            prevPage = $("#navitems li.selected a");

            $("#navitems li").removeClass("selected");
            $(this).parent("li").addClass("selected");

            if(toPage.length == 1 && toPage[0].tagName.toLowerCase() == "article") {
                event.preventDefault();
                $(window).scrollTop(0);
                $("article").hide();
                toPage.show();
                $(document).resize();
            }
}

function tooltip (event) {
    var target  = false,
        tooltip = false,
        tip     = false;

    if($("input, textarea").is(':focus') || $('.iw-contextMenu').css('display') == 'inline-block')
        return;

    event.stopPropagation();

    $("#tooltip").click();

    target  = $(this);
    tip     = target.attr('title');
    tooltip = $('<div id="tooltip"></div>');

    if(!tip || tip == '')
        return false;

    tip = tip.replace(/&#013;|\n|\x0A/g, '<br />')

    .replace(/%column%/g, function() {
        return $(target.parents("table").find("thead tr th")[target[0].cellIndex]).text();
    }).replace(/%([.\w\-]+),([.\w\-]+)%/g, function($0, $1, $2){
        return target.children($1).attr($2);
    }).replace(/%([.\w\-]+)%/g, function($0, $1){
        return target.children($1).text();
    });

    target.removeAttr('title');
    tooltip.css('opacity', 0)
           .html(tip)
           .appendTo('body');

    if(target.css('cursor') != "pointer" && target.prop("tagName") != "A")
        target.css('cursor', 'help');

    var init_tooltip = function() {
        if($(window).width() < tooltip.outerWidth() * 1.5)
            tooltip.css('max-width', $(window).width() / 2);
        else
            tooltip.css('max-width', 340);

        var pos_left = target.offset().left + (target.outerWidth() / 2) - (tooltip.outerWidth() / 2),
            pos_top  = target.offset().top - tooltip.outerHeight() - 20;

        if(pos_left < 0)
        {
            pos_left = target.offset().left + target.outerWidth() / 2 - 20;
            tooltip.addClass('left');
        }
        else
            tooltip.removeClass('left');

        if(pos_left + tooltip.outerWidth() > $(document).width())
        {
            pos_left = target.offset().left - tooltip.outerWidth() + target.outerWidth() / 2 + 13;
            tooltip.addClass('right');
        }
        else
            tooltip.removeClass('right');

        if(pos_left + target.outerWidth() > $(document).width())
        {
            pos_left = event.pageX;
            tooltip.removeClass('left right');
        }

        if(pos_top < 0)
        {
            var pos_top = target.offset().top + target.outerHeight();
            tooltip.addClass('top');
        }
        else
            tooltip.removeClass('top');

        tooltip.css({left: pos_left, top: pos_top})
               .animate({top: '+=10', opacity: 1}, 100);
    };

    init_tooltip();
    $(window).resize(init_tooltip);

    var remove_tooltip = function()
    {
        target.attr('title', tip);

        tooltip.animate({top: '-=10', opacity: 0}, 100, function() {
            $(this).remove();
        });
    };

    target.on('mouseleave', remove_tooltip);
    target.on('contextmenu', remove_tooltip);
    tooltip.on('click', remove_tooltip);
}


function connectSignals() {
    bridge.emitPaste.connect(this, pasteValue);

    bridge.emitTransactions.connect(this, appendTransactions);
    bridge.emitAddresses.connect(this, appendAddresses);
    bridge.emitMessages.connect(this, appendMessages);
    bridge.emitMessage.connect(this,  appendMessage);

    bridge.emitCoinControlUpdate.connect(this, updateCoinControlInfo);

    bridge.emitAddressBookReturn.connect(this, addressBookReturn);

    bridge.triggerElement.connect(this, triggerElement);

    bridge.emitReceipient.connect(this, addRecipientDetail);
    bridge.networkAlert.connect(this, networkAlert);

    optionsModel.displayUnitChanged.connect(unit, "setType");
    optionsModel.reserveBalanceChanged.connect(overviewPage, "updateReserved");
    optionsModel.rowsPerPageChanged.connect(this, "updateRowsPerPage");
    optionsModel.visibleTransactionsChanged.connect(this, "visibleTransactions");

    walletModel.encryptionStatusChanged.connect(overviewPage, "encryptionStatusChanged");
    walletModel.balanceChanged.connect(overviewPage, "updateBalance");

    overviewPage.clientInfo();
    optionsPage.update();
    chainDataPage.updateAnonOutputs();
}

function triggerElement(el, trigger) {
    $(el).trigger(trigger);
}

function updateRowsPerPage(rows) {
    $(".footable").each(function() {
        $(this).data().pageSize = rows;
        $(this).trigger('footable_initialize');
    });
}

var base58 = {
    base58Chars :"123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz",

    check: function(field)
    {
        var el = $(field);
        var value = el.val();

        for (var i = 0, len = value.length; i < len; ++i)
            if (base58.base58Chars.indexOf(value[i]) == -1) {
                el.css("background", "#0cb3db").css("color", "white");
                return false;
            }

        el.css("background", "").css("color", "");
        return true;
    }
}

var addressLookup = "",
    addressLabel  = "";

function openAddressBook(field, label, sending)
{
    addressLookup = field;
    addressLabel  = label;

    bridge.openAddressBook(sending);
}

function addressBookReturn(address, label)
{
    $(addressLookup).val(address).change();
    $(addressLabel) .val(label)  .change();
}

var pasteTo = "";

function pasteValue(value) {
    $(pasteTo).val(value);
}

function paste(field)
{
    pasteTo = field;
    bridge.paste();
    if (pasteTo.indexOf("#pay_to") == 0
        || pasteTo == '#change_address')
        base58.check(pasteTo);
}

function copy(field, attribute)
{
    var value = '';

    try {
        value = $(field).text();
    } catch(e) {};

    if(value==undefined||attribute!=undefined)
    {
        if(attribute=='copy')
            value = field;
        else
            value = $(field).attr(attribute);
    }

    bridge.copy(value);
}

function networkAlert(alert) {
    $("#network-alert span").text(alert);

    if(alert == "")
        $("#network-alert").hide();
    else
        $("#network-alert").show();
}

var unit = {
    type: 0,
    name: "GAMEUNITS",
    display: "GAMEUNITS",
    setType: function(type) {
        this.type = (type == undefined ? 0 : type);

        switch(type) {
            case 1:
                this.name = "mGAMEUNITS",
                this.display = "mGAMEUNITS";
                break;

            case 2:
                this.name = "uGAMEUNITS",
                this.display = "&micro;GAMEUNITS";
                break;

            case 3:
                this.name    = "sGAMEUNITS",
                this.display = "Gameunitsoshi";
                break;

            default:
                this.name = this.display = "GAMEUNITS";
        }

        $("td.unit,span.unit,div.unit").html(this.display);
        $("select.unit").val(type).trigger("change");
        $("input.unit").val(this.name);
        overviewPage.updateBalance();
    },
    format: function(value, type) {
        var el = ($.isNumeric(value) ? null : $(value));

        type  = (type == undefined ? this.type : parseInt(type)),
        value = parseInt(el == undefined ? value : (el.data('value') == undefined ? el.val() : el.data('value')));

        switch(type) {
            case 1: value = value / 100000; break;
            case 2: value = value / 100; break;
            case 3: break;
            default: value = value / 100000000;
        }

        value = value.toFixed(this.mask(type));

        if(el == undefined)
            return value;

        el.val(value);
    },
    parse: function(value, type) {
        var el = ($.isNumeric(value) ? null : $(value));

        type  = (type == undefined ? this.type : parseInt(type)),

        fp = (el == undefined ? value : el.val());
        if (fp == undefined || fp.length < 1)
            fp = ['0', '0'];
        else
        if (fp[0] == '.')
            fp = ['0', fp.slice(1)];
        else
            fp = fp.split('.');

        value = parseInt(fp[0]);

        var ipow = this.mask(type);
        if (ipow > 0)
            value *= Math.pow(10, ipow);
        if (ipow > 0 && fp.length > 1)
        {
            var av = fp[1].split('');

            while (av.length > 1 && av[av.length-1] == '0')
                av.pop();

            var fract = parseInt(av.join(''));

            if (fract > 0)
            {
                ipow -= av.length;

                if (ipow > 0)
                    fract = fract * Math.pow(10, ipow);
                value += fract
            };
        };

        if (el == undefined)
            return value;

        el.data('value', value);
        this.format(el, type);
    },
    mask: function(type) {
        type  = (type == undefined ? this.type : parseInt(type));

        switch(type) {
            case 1: return 5;
            case 2: return 2;
            case 3: return 0;
            default:return 8;
        }
    },
    keydown: function(e) {
        var key = e.which,
            type = $(e.target).siblings(".unit").val();


        if(key==190 || key == 110) {
            if(this.value.toString().indexOf('.') != -1 || unit.mask(type) == 0)
                e.preventDefault();

            return true;
        }

        if(!e.shiftKey && (key>=96 && key<=105 || key>=48 && key<=57)) {
            var selectS = this.selectionStart;
            var indP = this.value.indexOf(".");
            if (!(document.getSelection().type == "Range") && selectS > indP && this.value.indexOf('.') != -1 && this.value.length -1 - indP >= unit.mask(type))
            {
                if (this.value[this.value.length-1] == '0'
                    && selectS < this.value.length)
                {
                    this.value = this.value.slice(0,-1);
                    this.selectionStart = selectS;
                    this.selectionEnd = selectS;
                    return;
                }
                e.preventDefault();
            }
            return;
        }

        if(key==8||key==9||key == 17||key==46||key==45||key>=35 && key<=40||(e.ctrlKey && (key==65||key==67||key==86||key==88)))
            return;

        e.preventDefault();
    },
    paste: function(e) {
        var data = e.originalEvent.clipboardData.getData("text/plain");
        if(!($.isNumeric(data)) || (this.value.indexOf('.') != -1 && document.getSelection().type != "Range"))
            e.preventDefault();
    }
};

var contextMenus = [];
function openContextMenu(el)
{
    if (contextMenus.indexOf(el) == -1)
        contextMenus.push(el);

    if (el.isOpen != undefined && el.isOpen == 1)
    {
        el.isOpen = 0;
        if(el.close)
            el.close();
    }

    // -- close other menus (their onClose isn't called if they were closed by opening another memu)
    for (var i = 0; i < contextMenus.length; ++i)
        contextMenus[i].isOpen = contextMenus[i] == el ? 1 : 0;
}

/* Overview Page */
var overviewPage = {
    init: function() {
        this.balance = $(".balance"),
        this.gameunitsXBal = $("#gameunitsXBal"),
        this.reserved = $("#reserved"),
        this.stake = $("#stake"),
        this.unconfirmed = $("#unconfirmed"),
        this.immature = $("#immature"),
        this.total = $("#total");

        // Announcement feed
        $.ajax({
            url:"http://ajax.googleapis.com/ajax/services/feed/load?v=2.0&q=http://pastebin.com/raw/iQGUdzTn",
            dataType: 'jsonp'
        }).success(function(rss) {
            rss.responseData.feed.entries = rss.responseData.feed.entries.sort(function(a,b){
                return new Date(b.publishedDate) - new Date(a.publishedDate);
            });
            for(i=0;i<rss.responseData.feed.entries.length;i++) {
                $('#announcements').append("<h4><a href='" + rss.responseData.feed.entries[i].link  + "'>" + rss.responseData.feed.entries[i].title + "</a></h4>"
                                         + "<span>"
                                             +      new Date(rss.responseData.feed.entries[i].publishedDate).toDateString()
                                         + "</span>");
            }
        });

        var menu = [{
                name: 'Backup&nbsp;Wallet...',
                fa: 'fa-save red fa-fw',
                fun: function () {
                   bridge.userAction(['backupWallet']);
                }
                }, /*
                {
                    name: 'Export...',
                    img: 'qrc:///icons/editcopy',
                    fun: function () {
                        copy('#addressbook .footable .selected .label');
                    }
                }, */
                {
                    name: 'Sign&nbsp;Message...',
                    fa: 'fa-pencil-square-o red fa-fw',
                    fun: function () {
                       bridge.userAction({'signMessage': $('#receive .footable .selected .address').text()});
                    }
                },
                {
                    name: 'Verify&nbsp;Message...',
                    fa: 'fa-check red fa-fw',
                    fun: function () {
                       bridge.userAction({'verifyMessage': $('#addressbook .footable .selected .address').text()});
                    }
                },
                {
                    name: 'Exit',
                    fa: 'fa-times red fa-fw',
                    fun: function () {
                       bridge.userAction(['close']);
                    }
                }];

        $('#file').contextMenu(menu, {onOpen:function(data,e){openContextMenu(data.menu);}, onClose:function(data,e){data.menu.isOpen = 0;}, triggerOn: 'click', displayAround: 'trigger', position: 'bottom', mouseClick: 'left', sizeStyle: 'content'});

        menu = [{
                     id: 'encryptWallet',
                     name: 'Encrypt&nbsp;Wallet...',
                     fa: 'fa-lock red fa-fw',
                     fun: function () {
                        bridge.userAction(['encryptWallet']);
                     }
                 },
                 {
                     id: 'changePassphrase',
                     name: 'Change&nbsp;Passphrase...',
                     fa: 'fa-key red fa-fw',
                     fun: function () {
                        bridge.userAction(['changePassphrase']);
                     }
                 },
                 {
                     id: 'toggleLock',
                     name: '(Un)Lock&nbsp;Wallet...',
                     fa: 'fa-unlock red pad fa-fw',
                     fun: function () {
                        bridge.userAction(['toggleLock']);
                     }
                 },
                 {
                     name: 'Options',
                     fa: 'fa-wrench red fa-fw',
                     fun: function () {
                        $("#navitems [href=#options]").click();
                 		}
                    },
                {
					name: 'Backup&nbsp;Wallet...',
					fa: 'fa-save red fa-fw',
					fun: function () {
                   bridge.userAction(['backupWallet']);
				}
                    },
                {
                    name: 'Sign&nbsp;Message...',
                    fa: 'fa-pencil-square-o red fa-fw',
                    fun: function () {
                       bridge.userAction({'signMessage': $('#receive .footable .selected .address').text()});
                    }
                },
                {
                    name: 'Verify&nbsp;Message...',
                    fa: 'fa-check red fa-fw',
                    fun: function () {
                       bridge.userAction({'verifyMessage': $('#addressbook .footable .selected .address').text()});
                    }
                },
                {				
				     name: 'Debug&nbsp;Window...',
                     fa: 'fa-bug red fa-fw',
                     fun: function () {
                        bridge.userAction(['debugClicked']);
					}
                },
                {
                     name: ' About&nbsp;Gameunits',
                     fa: 'fa-caret-up red fa-fw',
                     fun: function () {
                        bridge.userAction(['aboutClicked']);
                     }
                 },
                 {
                     name: 'About&nbsp;Qt',
                     fa: 'fa-question red fa-fw',
                     fun: function () {
                        bridge.userAction(['aboutQtClicked']);
                     }
                 }];

        $('#settings').contextMenu(menu, {onOpen:function(data,e){openContextMenu(data.menu);}, onClose:function(data,e){data.menu.isOpen = 0;}, triggerOn: 'click', displayAround: 'trigger', position: 'bottom', mouseClick: 'left', sizeStyle: 'content'});

        menu = [{
                     name: 'Debug&nbsp;Window...',
                     fa: 'fa-bug red fa-fw',
                     fun: function () {
                        bridge.userAction(['debugClicked']);
                     }
                 },
                 {
                     name: 'Developer&nbsp;Tools...',
                     fa: 'fa-edit red fa-fw',
                     fun: function () {
                        bridge.userAction(['developerConsole']);
                     }
                 },
                 {
                     name: ' About&nbsp;Gameunits...',
                     img: 'qrc:///icons/gameunits',
                     fun: function () {
                        bridge.userAction(['aboutClicked']);
                     }
                 },
                 {
                     name: 'About&nbsp;Qt...',
                     fa: 'fa-question red fa-fw',
                     fun: function () {
                        bridge.userAction(['aboutQtClicked']);
                     }
                 }];

        $('#help').contextMenu(menu, {onOpen:function(data,e){openContextMenu(data.menu);}, onClose:function(data,e){data.menu.isOpen = 0;}, triggerOn: 'click', displayAround: 'trigger', position: 'bottom', mouseClick: 'left', sizeStyle: 'content'});
    },

    updateBalance: function(balance, gameunitsXBal, stake, unconfirmed, immature) {
        if(balance == undefined)
            balance     = this.balance    .data("orig"),
            gameunitsXBal   = this.gameunitsXBal  .data("orig"),
            stake       = this.stake      .data("orig"),
            unconfirmed = this.unconfirmed.data("orig"),
            immature    = this.immature   .data("orig");
        else
            this.balance    .data("orig", balance),
            this.gameunitsXBal  .data("orig", gameunitsXBal),
            this.stake      .data("orig", stake),
            this.unconfirmed.data("orig", unconfirmed),
            this.immature   .data("orig", immature);

        this.formatValue("balance",     balance);
        this.formatValue("gameunitsXBal",   gameunitsXBal);
        this.formatValue("stake",       stake);
        this.formatValue("unconfirmed", unconfirmed);
        this.formatValue("immature",    immature);
        this.formatValue("total", balance + stake + unconfirmed + immature);
        resizeFooter();
    },

    updateReserved: function(reserved) {
        this.formatValue("reserved", reserved);
    },

    formatValue: function(field, value) {

        if(field == "total" && value != undefined && !isNaN(value))
        {
            var val = unit.format(value).split(".");

            $("#total-big > span:first-child").text(val[0]);
            $("#total-big .cents").text(val[1]);
        }

        if(field == "stake" && value != undefined && !isNaN(value))
        {
            if(value == 0)
                $("#staking-big").addClass("not-staking");
            else
                $("#staking-big").removeClass("not-staking");

            var val = unit.format(value).split(".");

            $("#staking-big > span:first-child").text(val[0]);
            $("#staking-big .cents").text(val[1]);
        }

        field = this[field];

        if(value == 0) {
            field.html("");
            field.parent("tr").hide();
        } else {
            field.text(unit.format(value));
            field.parent("tr").show();
        }
    },
    recent: function(transactions) {
        for(var i = 0;i < transactions.length;i++)
            overviewPage.updateTransaction(transactions[i]);

        $("#recenttxns [title]").off("mouseenter").on("mouseenter", tooltip)
    },
    updateTransaction: function(txn) {
        var format = function(tx) {

            return "<a id='"+tx.id.substring(0,17)+"' title='"+tx.tt+"' class='transaction-overview' href='#' onclick='$(\"#navitems [href=#transactions]\").click();$(\"#"+tx.id+"\").click();'>\
                                                <span class='"+(tx.t == 'input' ? 'received' : (tx.t == 'output' ? 'sent' : (tx.t == 'inout' ? 'self' : 'stake')))+" icon no-padding blue-gameunits'>\
                                                  <i class='fa fa-"+(tx.t == 'input' ? 'angle-left' : (tx.t == 'output' ? 'angle-right' : (tx.t == 'inout' ? 'angle-down' : 'caret-up')))+" font-26px margin-right-10'></i>"
                                                +unit.format(tx.am)+" </span> <span> "+unit.display+" </span> <span class='overview_date' data-value='"+tx.d+"'>"+tx.d_s+"</span></a>";

        }

        var sid = txn.id.substring(0,17);

        if($("#"+sid).attr("title", txn.tt).length==0)
        {
            var set = $('#recenttxns a');
            var txnHtml = format(txn);

            var appended = false;

            for(var i = 0; i<set.length;i++)
            {
                var el = $(set[i]);

                if (parseInt(txn.d) > parseInt(el.find('.overview_date').data("value")))
                {
                    el.before(txnHtml);
                    appended = true;
                    break;
                }
            }

            if(!appended)
                $("#recenttxns").append(txnHtml);

            set = $('#recenttxns a');

            while(set.length > 7)
            {
                $("#recenttxns a:last").remove();

                set = $('#recenttxns a');
            }
        }

        /*
        if (set.length == 0)
        {
            $("#recenttxns").append(format(txn));

            //return;
        }

        var sid = txn.id.substring(0,17);

        set.each(function(index) {
            var el = $(this);
            console.log(index);
            if (txn.date > el.find('.overview_date').attr("data-value"))
                el.before(format(txn));
            else
                el.after(format(txn));

            if (set.length >= 7)
                $("#recenttxns a:last").remove();

            return;
        });*/
    },
    clientInfo: function() {
        $('#version').text(bridge.info.build.replace(/\-[\w\d]*$/, ''));
        $('#clientinfo').attr('title', 'Build Desc: ' + bridge.info.build + '\nBuild Date: ' + bridge.info.date).on('mouseenter', tooltip);
    },
    encryptionStatusChanged: function(status) {
        switch(status)
        {
        case 0: // Not Encrypted
        case 1: // Unlocked
        case 2: // Locked
        }
    }
}

var optionsPage = {
    init: function() {
    },

    update: function() {
        var options = bridge.info.options;
        $("#options-ok,#options-apply").addClass("disabled");

        for(var option in options)
        {
            var element = $("#opt"+option),
                value   = options[option],
                values  = options["opt"+option];

            if(element.length == 0)
            {
                if(option.indexOf('opt') == -1)
                    console.log('Option element not available for %s', option);

                continue;
            }

            if(values)
            {
                element.html("");

                for(var prop in values)
                    if(typeof prop == "string" && $.isArray(values[prop]) && !$.isNumeric(prop))
                    {
                        element.append("<optgroup label='"+prop[0].toUpperCase() + prop.slice(1)+"'>");

                        for(var i=0;i<values[prop].length;i++)
                            element.append("<option>" + values[prop][i] + "</option>");
                    }
                    else
                            element.append("<option" + ($.isNumeric(prop) ? '' : " value='"+prop+"'") + ">" + values[prop] + "</option>");
            }

            function toggleLinked(el) {
                el = $(this);
                var enabled = el.prop("checked"),
                    linked = el.data("linked");

                if(linked)
                    linked = linked.split(" ");
                else
                    return;

                for(var i=0;i<linked.length;i++)
                {
                    var linkedElements = $("#"+linked[i]+",[for="+linked[i]+"]").attr("disabled", !enabled);
                    if(enabled)
                        linkedElements.removeClass("disabled");
                    else
                        linkedElements.addClass("disabled");
                }
            }

            if(element.is(":checkbox"))
            {
                element.prop("checked", value == true||value == "true");
                element.off("change");
                element.on("change", toggleLinked);
                element.change();
            }
            else if(element.is("select[multiple]") && value == "*")
                element.find("option").attr("selected", true);
            else
                element.val(value);

            element.one("change", function() {$("#options-ok,#options-apply").removeClass("disabled");});
        }
    },
    save: function() {
        var options = bridge.info.options,
            changed = {};

        for(var option in options)
        {
            var element  = $("#opt"+option),
                oldvalue = options[option],
                newvalue = false;

            if(oldvalue == null || oldvalue == "false")
                oldvalue = false;

            if(element.length == 0)
                continue;

            if(element.is(":checkbox"))
                newvalue = element.prop("checked");
            else if(element.is("select[multiple]") && element.find("option:not(:selected)").length == 0)
                newvalue = "*";
            else
                newvalue = element.val();

            if(oldvalue != newvalue && oldvalue.toString() != newvalue.toString())
                changed[option] = newvalue;
        }

        if(!$.isEmptyObject(changed))
        {
            bridge.userAction({'optionsChanged': changed});
            optionsPage.update();

            if(changed.hasOwnProperty('AutoRingSize'))
                changeTxnType();
        }
    }
}

/* Send Page */
function sendPageInit() {
    toggleCoinControl(); // TODO: Send correct option value...
    addRecipient();
    changeTxnType();
}

var recipients = 0;

function addRecipient() {

    $("#recipients").append((
           (recipients == 0 || $("div.recipient").length == 0 ? '' : '<hr />')
        +  '<div id="recipient[count]" class="recipient"> \
            <div class="flex-right"> \
                <label for="pay_to[count]" class="recipient">Pay To:</label> \
                <input id="pay_to[count]" class="pay_to input_box" title="The address to send the payment to  (e.g. MXywGBZBowrppUwwNUo1GCRDTibzJi7g2M)" placeholder="Enter a Gameunits address (e.g. MXywGBZBowrppUwwNUo1GCRDTibzJi7g2M)" maxlength="128" oninput="base58.check(this);" onchange="$(\'#label[count]\').val(bridge.getAddressLabel(this.value));"/> \
                <a class="button is-inverse has-fixed-icon" title="Choose address from address book" style="margin-right:10px; margin-left:10px; height:43px; width:43px;" onclick="openAddressBook(\'#pay_to[count]\', \'#label[count]\', true)"><i class="fa fa-book"></i></a> \
                <a class="button is-inverse has-fixed-icon" title="Paste address from clipboard" style="margin-right:10px; height:43px; width:43px;" onclick="paste(\'#pay_to[count]\')"><i class="fa fa-files-o"></i></a> \
                <a class="button is-inverse has-fixed-icon" title="Remove this recipient" style="height:43px; width:43px;" onclick="if($(\'div.recipient\').length == 1) clearRecipients(); else {var recipient=$(\'#recipient[count]\');if(recipient.next(\'hr\').remove().length==0)recipient.prev(\'hr\').remove();$(\'#recipient[count]\').remove();resizeFooter();}"><i class="fa fa-times"></i></a> \
            </div> \
            <div class="flex-right"> \
                <label for="label[count]" class="recipient">Label:</label> \
                <input id="label[count]" class="label input_box" title="Enter a label for this address to add it to your address book" placeholder="Enter a label for this address to add it to your address book" maxlength="128"/> \
            </div> \
            <div class="flex-right"> \
                <label for="narration[count]" class="recipient">Narration:</label> \
                <input id="narration[count]" class="narration input_box" title="Enter a short note to send with payment (max 24 characters)" placeholder="Enter a short note to send with a payment (max 24 characters)" maxlength="24" /> \
            </div> \
            <div class="flex-left"> \
                <label for="amount[count]" class="recipient">Amount:</label> \
                <input id="amount[count]" class="amount input_box" type="number" placeholder="0.00000000" step="0.01" value="0.00000000" onfocus="invalid($(this), true);" onchange="unit.parse(this, $(\'#unit[count]\').val());updateCoinControl();"  /> \
                <select id="unit[count]" class="unit button is-inverse has-fixed-icon"  style="margin-left:10px; height:43px; width:100px;" onchange="unit.format(\'#amount[count]\', $(this).val());"> \
                    <option value="0" title="Gameunits"                    ' + (unit.type == 0 ? "selected" : "") + '>GAMEUNITS</option> \
                    <option value="1" title="Milli-Gameunits (1 / 1000)"   ' + (unit.type == 1 ? "selected" : "") + '>mGAMEUNITS</option> \
                    <option value="2" title="Micro-Gameunits (1 / 1000000)"' + (unit.type == 2 ? "selected" : "") + '>&micro;GAMEUNITS</option> \
                    <option value="3" title="Gameunitsoshi (1 / 100000000)" ' + (unit.type == 3 ? "selected" : "") + '>Gameunitsoshi</option> \
                </select> \
            </div> \
        </div>').replace(/\[count\]/g, recipients++));
        resizeFooter();

        // Don't allow characters in numeric fields
        $("#amount"+(recipients-1).toString()).on("keydown", unit.keydown).on("paste",  unit.paste);

        // Addressbook Modal
        $("#addressbook"+(recipients-1).toString()).leanModal({ top : 10, left: 5, overlay : 0.5, closeButton: ".modal_close" });

    bridge.userAction(['clearRecipients']);
}

function clearRecipients() {
    $("#recipients").html("");
    recipients = 0;
    addRecipient();
    resizeFooter();
}

function addRecipientDetail(address, label, narration, amount) {
    var recipient = recipients - 1;

    $("#pay_to"+recipient).val(address).change();
    $("#label"+recipient).val(label).change();
    $("#narration"+recipient).val(narration).change();
    $("#amount"+recipient).val(amount).change();
}

function changeTxnType()
{
    var type=$("#txn_type").val();

    if (type > 1)
    {
        if(bridge.info.options.AutoRingSize == true)
        {
            $("#tx_ringsize").hide();
            $("#suggest_ring_size").hide();
        } else
        {
            $("#tx_ringsize").show();
            $("#suggest_ring_size").show();
        }
        $("#coincontrol").hide();
    }
    else
    {
        $("#tx_ringsize").hide();
        $("#suggest_ring_size").hide();
        $("#coincontrol").show();
    }

    resizeFooter();
}

function suggestRingSize()
{
    chainDataPage.updateAnonOutputs();

    var minsize = bridge.info.options.MinRingSize||3,
        maxsize = bridge.info.options.MaxRingSize||100;

    function mature(value, min_owned) {
        if(min_owned == undefined || !$.isNumeric(min_owned))
            min_owned = 1;

        var anonOutput = chainDataPage.anonOutputs[value];

        if(anonOutput)
            return Math.min(anonOutput
               && anonOutput.owned_mature  >= min_owned
               && anonOutput.system_mature >= minsize
               && anonOutput.system_mature, maxsize);
        else
            return 0;
    }

    function getOutputRingSize(output, test, maxsize)
    {
        switch (output)
        {
            case 0:
                return maxsize;
            case 2:
                return mature(1*test, 2)||getOutputRingSize(++output, test, maxsize);
            case 6:
                return Math.min(mature(5*test, 1),
                                mature(1*test, 1))||getOutputRingSize(++output, test, maxsize);
            case 7:
                return Math.min(mature(4*test, 1),
                                mature(3*test, 1))||getOutputRingSize(++output, test, maxsize);
            case 8:
                return Math.min(mature(5*test, 1),
                                mature(3*test, 1))||getOutputRingSize(++output, test, maxsize);
            case 9:
                return Math.min(mature(5*test, 1),
                                mature(4*test, 1))||getOutputRingSize(++output, test, maxsize);
            default:
                if(output == 10)
                    return mature(test/2, 2);

                maxsize = Math.max(mature(output*test, 1),mature(1*test, output))||getOutputRingSize(output==1?3:++output, test, maxsize);
        }
        return maxsize;
    }

    for(var i=0;i<recipients;i++)
    {
        var test = 1,
            output = 0,
            el = $("#amount"+i),
            amount = unit.parse(el.val(), $("#unit"+i));

        $("[name=err"+el.attr('id')+"]").remove();

        while (amount >= test && maxsize >= minsize)
        {
            output = parseInt((amount / test) % 10);
            try {
                maxsize = getOutputRingSize(output, test, maxsize);
            } catch(e) {
                console.log(e);
            } finally {
                if(!maxsize)
                    maxsize = mature(output*test);

                test *= 10;
            }
        }

        if(maxsize < minsize)
        {
            invalid(el);
            el.parent().after("<div name='err"+el.attr('id')+"' class='warning'>Not enough system and or owned outputs for the requested amount.<br><br>Only <b>"
                     +maxsize+"</b> anonymous outputs exist for coin value: <b>" + unit.format(output*(test/10), $("#unit"+i)) + "</b></div>");
            el.on('change', function(){$("[name=err"+el.attr('id')+"]").remove();});

            $("#tx_ringsize").show();
            $("#suggest_ring_size").show();

            return;
        }
    }
    $("#ring_size").val(maxsize);
}

function toggleCoinControl(enable) {
    if(enable==undefined && $("#coincontrol_enabled")  .css("display") == "block" || enable == false)
    {
        $("#coincontrol_enabled") .css("display", "none");
        $("#coincontrol_disabled").css("display", "block");
    } else
    {
        $("#coincontrol_enabled") .css("display", "block");
        $("#coincontrol_disabled").css("display", "none");
    }
    resizeFooter();
}

function updateCoinControl() {
    if($("#coincontrol_enabled").css("display") == "none")
        return;
    var amount = 0;

    for(var i=0;i<recipients;i++)
        amount += unit.parse($("#amount"+i).val());

    bridge.updateCoinControlAmount(amount);
}

function updateCoinControlInfo(quantity, amount, fee, afterfee, bytes, priority, low, change)
{
    if($("#coincontrol_enabled").css("display") == "none")
        return;

    if (quantity > 0)
    {
        $("#coincontrol_auto").hide();

        var enable_change = (change == "" ? false : true);

        $("#coincontrol_quantity").text(quantity);
        $("#coincontrol_amount")  .text(unit.format(amount));
        $("#coincontrol_fee")     .text(unit.format(fee));
        $("#coincontrol_afterfee").text(unit.format(afterfee));
        $("#coincontrol_bytes")   .text("~"+bytes).css("color", (bytes > 10000 ? "red" : null));
        $("#coincontrol_priority").text(priority).css("color", (priority.indexOf("low") == 0 ? "red" : null)); // TODO: Translations of low...
        $("#coincontrol_low")     .text(low).toggle(enable_change).css("color", (low == "yes" ? "red" : null)); // TODO: Translations of low outputs
        $("#coincontrol_change")  .text(unit.format(change)).toggle(enable_change);

        $("label[for='coincontrol_low']")   .toggle(enable_change);
        $("label[for='coincontrol_change']").toggle(enable_change);

        $("#coincontrol_labels").show();

    } else
    {
        $("#coincontrol_auto").show();
        $("#coincontrol_labels").hide();
        $("#coincontrol_quantity").text("");
        $("#coincontrol_amount")  .text("");
        $("#coincontrol_fee")     .text("");
        $("#coincontrol_afterfee").text("");
        $("#coincontrol_bytes")   .text("");
        $("#coincontrol_priority").text("");
        $("#coincontrol_low")     .text("");
        $("#coincontrol_change")  .text("");
    }
}

var invalid = function(el, valid) {
    if(valid == true)
        el.css("background", "").css("color", "");
    else
        el.css("background", "#0cb3db").css("color", "white");

    return (valid == true);
}

function sendCoins() {
    bridge.userAction(['clearRecipients']);

    if(bridge.info.options.AutoRingSize && $("#txn_type").val() > 1)
        suggestRingSize();

    for(var i=0;i<recipients;i++) {
        var el = $("#pay_to"+i);
        var valid = true;

        valid = invalid(el, bridge.validateAddress(el.val()));

        el = $("#amount"+i);

        if(unit.parse(el.val()) == 0 && !invalid(el))
            valid = false;

        if(!valid || !bridge.addRecipient($("#pay_to"+i).val(), $("#label"+i).val(), $("#narration"+i).val(), unit.parse($("#amount"+i).val(), $("#unit"+i).val()), $("#txn_type").val(), $("#ring_size").val()))
            return false;
    }

    if(bridge.sendCoins($("#coincontrol_enabled").css("display") != "none", $("#change_address").val()))
        clearRecipients();
}

function receivePageInit() {
    var menu = [{
            name: 'Copy&nbsp;Address',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#receive .footable .selected .address');
            }
        },
        {
            name: 'Copy&nbsp;Label',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#receive .footable .selected .label');
            }
        },
        {
            name: 'Copy&nbsp;Public&nbsp;Key',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#receive .footable .selected .pubkey');
            }
        },
        {
            name: 'Edit',
            img: 'qrc:///icons/edit',
            fun: function () {
                $("#receive .footable .selected .label.editable").dblclick();
            }
        },
        {
            name: 'Show&nbsp;QR&nbsp;Code',
            img: 'qrc:///icons/qrcode',
            fun: function () {
                $("#receive [href='#qrcode-modal']").click();
            }
        },
        {
            name: 'Sign&nbsp;Message',
            img: 'qrc:///icons/edit',
            fun: function () {
                bridge.userAction({'signMessage': $('#receive .footable .selected .address').text()});
            }
        }];

    //Calling context menu
     $('#receive .footable tbody').on('contextmenu', function(e) {
        $(e.target).closest('tr').click();
     }).contextMenu(menu, {triggerOn:'contextmenu', sizeStyle: 'content'});
}


function addressBookInit() {
    var menu = [{
            name: 'Copy&nbsp;Address',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#addressbook .footable .selected .address');
            }
        },
        {
            name: 'Copy&nbsp;Public&nbsp;Key',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#addressbook .footable .selected .pubkey');
            }
        },
        {
            name: 'Copy&nbsp;Label',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#addressbook .footable .selected .label');
            }
        },
        {
            name: 'Edit',
            img: 'qrc:///icons/edit',
            fun: function () {
                $("#addressbook .footable .selected .label.editable").dblclick();
            }
        },
        {
            name: 'Delete',
            img: 'qrc:///icons/delete',
            fun: function () {
                var addr=$('#addressbook .footable .selected .address');
                if(bridge.deleteAddress(addr.text()))
                    addr.closest('tr').remove();

                resizeFooter();
            }
        },
        {
            name: 'Show&nbsp;QR&nbsp;Code',
            img: 'qrc:///icons/qrcode',
            fun: function () {
                $("#addressbook [href='#qrcode-modal']").click();
            }
        },
        {
            name: 'Verify&nbsp;Message',
            img: 'qrc:///icons/edit',
            fun: function () {
                bridge.userAction({'verifyMessage': $('#addressbook .footable .selected .address').text()});
            }
        }];

    //Calling context menu
     $('#addressbook .footable tbody').on('contextmenu', function(e) {
        $(e.target).closest('tr').click();
     }).contextMenu(menu, {triggerOn:'contextmenu', sizeStyle: 'content'});
}


var Name = 'Me';
var initialAddress = true;

function appendAddresses(addresses) {

    if(typeof addresses == "string")
    {
        if(addresses == "[]")
            return;

        addresses = JSON.parse(addresses.replace(/,\]$/, "]"));
    }

    for(var i=0; i< addresses.length;i++)
    {
        var address = addresses[i];
        var addrRow = $("#"+address.address);
        var page = (address.type == "S" ? "#addressbook" : "#receive");

        if(address.type == "R" && address.address.length < 75) {
            if(addrRow.length==0)
                $("#message-from-address").append("<option title='"+address.address+"' value='"+address.address+"'>"+address.label+"</option>");
            else
                $("#message-from-address option[value="+address.address+"]").text(address.label);

            if(initialAddress) {
                $("#message-from-address").prepend("<option title='Anonymous' value='anon' selected>Anonymous</option>");

                $(".user-name")   .text(Name);
                $(".user-address").text(address.address);
                initialAddress = false;
            }
        }

        if(addrRow.length==0)
        {
            $(page + " .footable tbody").append("<tr id='"+address.address+"'>\
                                                   <td class='label editable' data-value='"+address.label_value+"'>"+address.label+"</td>\
                                                   <td class='address'>"+address.address+"</td>\
                                                   <td class='pubkey'>"+address.pubkey+"</td></tr>");

            $("#"+address.address)

            .on('click', function() {
                $(this).addClass("selected").siblings("tr").removeClass("selected");
            }).find(".editable").on("dblclick", function (event) {
                event.stopPropagation();
                updateValue($(this));
            }).attr("title", "Double click to edit").on('mouseenter', tooltip);
        }
        else
        {

            $("#"+address.address+" .label") .data("value", address.label_value).text(address.label);
            $("#"+address.address+" .pubkey").text(address.pubkey);
        }

    }

    var table = $('#addressbook .footable,#receive .footable').trigger("footable_setup_paging");
}

function transactionPageInit() {
    var menu = [{
            name: 'Copy&nbsp;Address',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#transactions .footable .selected .address', "data-value");
            }
        },
        {
            name: 'Copy&nbsp;Label',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#transactions .footable .selected .address', "data-label");
            }
        },
        {
            name: 'Copy&nbsp;Amount',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#transactions .footable .selected .amount');
            }
        },
        {
            name: 'Copy&nbsp;transaction&nbsp;ID',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#transactions .footable .selected', "id");
            }
        },
        {
            name: 'Edit&nbsp;label',
            img: 'qrc:///icons/edit',
            fun: function () {
                $("#transactions .footable .selected .address .editable").dblclick();
            }
        },
        {
            name: 'Show&nbsp;transaction&nbsp;details',
            img: 'qrc:///icons/history',
            fun: function () {
                $("#transactions .footable .selected").dblclick();
            }
        }];

    //Calling context menu
     $('#transactions .footable tbody').on('contextmenu', function(e) {
        $(e.target).closest('tr').click();
     }).contextMenu(menu, {triggerOn:'contextmenu', sizeStyle: 'content'});

    $('#transactions .footable').on("footable_paging", function(e) {
        var transactions = filteredTransactions.slice(e.page * e.size)
            transactions = transactions.slice(0, e.size);

        var $tbody = $("#transactions .footable tbody");

        $tbody.html("");

        delete e.ft.pageInfo.pages[e.page];

        e.ft.pageInfo.pages[e.page] = transactions.map(function(val) {
            val.html = formatTransaction(val);

            $tbody.append(val.html);

            return $("#"+val.id)[0];
        });
        e.result = true;

        bindTransactionTableEvents();
        resizeFooter();

    }).on("footable_create_pages", function(e) {
        var $txtable = $("#transactions .footable");
        if(!$($txtable.data("filter")).val())
            filteredTransactions = Transactions;

        /* Sort Columns */
        var sortCol = $txtable.data("sorted"),
            sortAsc = $txtable.find("th.footable-sorted").length == 1,
            sortFun = 'numeric';

        switch(sortCol)
        {
        case 0:
            sortCol = 'c';
            break;
        case 2:
            sortCol = 't_l',
            sortFun = 'alpha';
            break;
        case 3:
            sortCol = 'ad',
            sortFun = 'alpha';
            break;
        case 4:
            sortCol = 'n',
            sortFun = 'alpha';
            break;
        case 5:
            sortCol = 'am';
            break;
        default:
            sortCol = 'd';
            break;
        }

        sortFun = e.ft.options.sorters[sortFun];

        filteredTransactions.sort(function(a, b) {
            return sortAsc ? sortFun(a[sortCol], b[sortCol]) : sortFun(b[sortCol], a[sortCol]);
        });
        /* End - Sort Columns */

        /* Add pages */
        delete e.ft.pageInfo.pages;
        e.ft.pageInfo.pages = [];
        var addPages = Math.ceil(filteredTransactions.length / e.ft.pageInfo.pageSize),
            newPage  = [];

        if(addPages > 0)
        {
            for(var i=0;i<e.ft.pageInfo.pageSize;i++)
                newPage.push([]);

            for(var i=0;i<addPages;i++)
                e.ft.pageInfo.pages.push(newPage);
        }

        /* End - Add pages */
    }).on("footable_filtering", function(e) {
        if(e.clear)
            return true;

        filteredTransactions = Transactions.filter(function(transaction) {
            for(var prop in transaction)
                if(transaction[prop].toString().toLowerCase().indexOf(e.filter.toLowerCase()) != -1)
                    return true;

            return false;
        });
    });
}


var Transactions = [],
    filteredTransactions = [];

function formatTransaction(transaction) {
    return "<tr id='"+transaction.id+"' title='"+transaction.tt+"'>\
                    <td data-value='"+transaction.c+"'><i class='fa fa-lg "+transaction.s+" margin-right-10'></td>\
                    <td data-value='"+transaction.d+"'>"+transaction.d_s+"</td>\
                    <td>"+transaction.t_l+"</td>\
                    <td class='address' style='color:"+transaction.a_c+";' data-value='"+transaction.ad+"' data-label='"+transaction.ad_l+"'><img src='qrc:///icons/tx_"+transaction.t+"' /><span class='editable'>"+transaction.ad_d+"</span></td>\
                    <td>"+transaction.n+"</td>\
                    <td class='amount' style='color:"+transaction.am_c+";' data-value='"+transaction.am+"'>"+transaction.am_d+"</td>\
                 </tr>";
}

function visibleTransactions(visible) {
    if(visible[0] != "*")
        Transactions = Transactions.filter(function(val) {
            return this.some(function(val){return val == this}, val.t_l);
        }, visible);
}

function bindTransactionTableEvents() {

    $("#transactions .footable tbody tr")
    .on('mouseenter', tooltip)

    .on('click', function() {
        $(this).addClass("selected").siblings("tr").removeClass("selected");
    })

    .on("dblclick", function(e) {
        $(this).attr("href", "#transaction-info-modal");

        $(this).leanModal({ top : 10, overlay : 0.5, closeButton: "#transaction-info-modal .modal_close" });
        $("#transaction-info").html(bridge.transactionDetails($(this).attr("id")));
        $(this).click();

        $(this).off('click');
        $(this).on('click', function() {
                $(this).addClass("selected").siblings("tr").removeClass("selected");
        })
    }).find(".editable")

   .on("dblclick", function (event) {
      event.stopPropagation();
      event.preventDefault();
      updateValue($(this));
   }).attr("title", "Double click to edit").on('mouseenter', tooltip);
}

function appendTransactions(transactions) {
    if(typeof transactions == "string")
    {
        if(transactions == "[]")
            return;

        transactions = JSON.parse(transactions.replace(/,\]$/, "]"));
    }

    if(transactions.length==1 && transactions[0].id==-1)
        return;

    transactions.sort(function (a, b) {
        a.d = parseInt(a.d);
        b.d = parseInt(b.d);

        return b.d - a.d;
    });

    Transactions = Transactions.filter(function(val) {
        return this.some(function(val) {
            return val.id == this.id;
        }, val) == false;
    }, transactions)
    .concat(transactions);

    overviewPage.recent(transactions.slice(0,7));

    $("#transactions .footable").trigger("footable_redraw");
}

function ChatInit() {
    var menu = [{
            name: 'Send&nbsp;Gameunits',
            fun: function () {
                clearRecipients();
                $("#pay_to0").val($('#contact-list .selected .contact-address').text());
                $("#navpanel [href=#send]").click();
            }
        },
        {
            name: 'Copy&nbsp;Address',
            fun: function () {
                copy('#contact-list .selected .contact-address');
            }
        },
        /*
        {
            name: 'Send&nbsp;File',
            img: 'qrc:///icons/editcopy',
            fun: function () {
                copy('#transactions .footable .selected .address', "data-label");
            }
        },*/
        {
            name: 'Private&nbsp;Message',
            fun: function () {
                $("#message-text").focus();
            }
        } /*,
        {
            name: 'Edit&nbsp;Label',
            fun: function () {
                console.log("todo"); //TODO: this...
            }
        } /*,
        {
            name: 'Block&nbsp;Address',
            fun: function () {
                console.log("todo"); //TODO: Blacklist...
            }
        }*/
        ];

    //Calling context menu
    $('#contact-list').on('contextmenu', function(e) {
       $(e.target).closest('li').click();
    }).contextMenu(menu, {triggerOn:'contextmenu', sizeStyle: 'content'});

    menu = [{
            name: 'Copy&nbsp;Message',
            fun: function () {
                var selected = $(".contact-discussion li.selected"),
                    id = selected.attr("id");

                $.each(contacts[selected.attr("contact-key")].messages, function(index){if(this.id == id) copy(this.message, 'copy');});
            }
        },
        /*
        {
            name: 'Send&nbsp;File',
            fun: function () {
                copy('#transactions .footable .selected .address', "data-label");
            }
        },*/
        {
            name: 'Delete&nbsp;Message',
            fun: function () {
                $(".contact-discussion li.selected").find(".delete").click();
            }
        }];

    $('.contact-discussion').on('contextmenu', function(e) {
        $(e.target).closest('li').addClass("selected");
    }).contextMenu(menu, {triggerOn:'contextmenu', sizeStyle: 'content'});

    menu = [
        {
            name: 'Copy&nbsp;Selected',
            fun: function () {
                var editor = $("#message-text")[0];

                if (typeof editor.selectionStart !== 'undefined')
                    copy(editor.value.substring(editor.selectionStart, editor.selectionEnd), 'copy');
            }
        },
        {
            name: 'Paste',
            fun: function () {
                paste("#pasteTo");

                var editor = $("#message-text")[0];

                if(typeof editor.selectionStart !== 'undefined')
                    editor.value = editor.value.substring(editor.selectionStart, 0) + $("#pasteTo").val() + editor.value.substring(editor.selectionStart);
                else
                    editor.value += $("#pasteTo").val();
            }
        }];

    $('#message-text').contextMenu(menu, {triggerOn:'contextmenu', sizeStyle: 'content'});
}

var contacts = {};
var contact_list;

function appendMessages(messages, reset) {
    contact_list = $("#contact-list ul");

    if(reset)
    {
        contacts = null;
        contacts = {};
        contact_list.html("");
        $("#contact-list").removeClass("in-conversation");
        $(".contact-discussion ul").html("");
        $(".user-notifications").hide();
        $("#message-count").text(0);
        messagesScroller.scrollTo(0, 0);
        contactScroll   .scrollTo(0, 0);
    }

    if(messages == "[]")
        return;

    messages = JSON.parse(messages.replace(/,\]$/, "]"));

    // Massage data
    for(var i=0; i<messages.length; i++)
    {
        var message = messages[i];
        appendMessage(message.id,
                      message.type,
                      message.sent_date,
                      message.received_date,
                      message.label_value,
                      message.label,
                      message.to_address,
                      message.from_address,
                      message.read,
                      message.message,
                      true);
    }

    for (key in contacts) {
        appendContact(key);
    }

    contact_list.find("li:first-child").click();

}

function appendMessage(id, type, sent_date, received_date, label_value, label, to_address, from_address, read, message, initial) {
    if(type=="R"&&read==false) {
        $(".user-notifications").show();
        $("#message-count").text(parseInt($("#message-count").text())+1);
    }

    var them = type == "S" ? to_address   : from_address;
    var self = type == "S" ? from_address : to_address;

    var key = (label_value == "" ? them : label_value).replace(/\s/g, '');

    var contact = contacts[key];

    if(contacts[key] == undefined)
        contacts[key] = {},
        contact = contacts[key],
        contact.key = key,
        contact.label = label,
        contact.avatar = (false ? '' : 'qrc:///images/default'), // TODO: Avatars!!
        contact.messages  = new Array();

    if($.grep(contact.messages, function(a){ return a.id == id; }).length == 0)
    {
        contact.messages.push({id:id, them: them, self: self, message: message, type: type, sent: sent_date, received: received_date, read: read});

        if(!initial)
            appendContact(key, true);
    }
}

function appendContact (key, newcontact) {
    var contact_el = $("#contact-"+key);
    var contact = contacts[key];

    var unread_count = $.grep(contact.messages, function(a){return a.type=="R"&&a.read==false}).length;

    if(contact_el.length == 0) {
        contact_list.append("<li id='contact-"+ key +"' class='contact' title='"+contact.label+"'>\
                                        <img src='"+ contact.avatar +"' />\
                                        <span class='contact-info'>\
                                            <span class='contact-name'>"+contact.label+"</span>\
                                            <span class='contact-address'>"+contact.messages[0].them+"</span>\
                                        </span>\
                                        <span class='contact-options'>\
                                                <span class='message-notifications"+(unread_count==0?' none':'')+"'>"+unread_count+"</span>\
                                                <span class='delete' onclick='deleteMessages(\""+key+"\")'></span>\
                                                " //<span class='favorite favorited'></span>\ //TODO: Favourites
                                     + "</span>\
                                      </li>");

        contact_el = $("#contact-"+key).on('click', function(e) {
            $(this).addClass("selected").siblings("li").removeClass("selected");
            $("#contact-list").addClass("in-conversation");
            var discussion = $(".contact-discussion ul");
            var contact = contacts[e.delegateTarget.id.replace(/^contact\-/, '')];

            discussion.html("");

            contact.messages.sort(function (a, b) {
              return a.received - b.received;
            });

            var message;

            for(var i=0;i<contact.messages.length;i++)
            {
                message = contact.messages[i];
                if(message.read == false && bridge.markMessageAsRead(message.id))
                {
                    var message_count = $("#message-count"),
                        message_count_val = parseInt(message_count.text())-1;

                    message_count.text(message_count_val);
                    if(message_count_val==0)
                        message_count.hide();
                    else
                        message_count.show();
                }

                //title='"+(message.type=='S'? message.self : message.them)+"' taken out below.. titles getting in the way..
                discussion.append("<li id='"+message.id+"' class='"+(message.type=='S'?'user-message':'other-message')+"' contact-key='"+contact.key+"'>\
                                    <span class='info'>\
                                        <img src='"+contact.avatar+"' />\
                                        <span class='user-name'>"
                                            +(message.type=='S'? (message.self == 'anon' ? 'anon' : Name) : contact.label)+"\
                                        </span>\
                                    </span>\
                                    <span class='message-content'>\
                                        <span class='timestamp'>"+(new Date(message.received*1000).toLocaleString())+"</span>\
                                        <span class='message-text'>"+micromarkdown.parse(message.message)+"</span>\
                                        <span class='delete' onclick='deleteMessages(\""+contact.key+"\", \""+message.id+"\");'></span>\
                                    </span></li>");

            }


            messagesScroller.refresh();

            messagesScroller.scrollTo(0, messagesScroller.maxScrollY, 600);

            var scrollerBottom = function() {

                var max = messagesScroller.maxScrollY;

                messagesScroller.refresh();

                if(max != messagesScroller.maxScrollY)
                    messagesScroller.scrollTo(0, messagesScroller.maxScrollY, 100);
            };

            setTimeout(scrollerBottom, 700);
            setTimeout(scrollerBottom, 1000);
            setTimeout(scrollerBottom, 1300);
            setTimeout(scrollerBottom, 1600);
            setTimeout(scrollerBottom, 1900);
            setTimeout(scrollerBottom, 2200);
            setTimeout(scrollerBottom, 2500);
            setTimeout(scrollerBottom, 5000);

            //discussion.children("[title]").on("mouseenter", tooltip);

            $("#message-from-address").val(message.self);
            $("#message-to-address").val(message.them);

        }).on("mouseenter", tooltip);

        contact_el.find(".delete").on("click", function(e) {e.stopPropagation()});

    } else {
        var received_message = contact.messages[contact.messages.length-1];

        if(received_message.type=="R"&&received_message.read==false) {
            var notifications = contact_el.find(".message-notifications");
            notifications.text(unread_count);
        }
    }

    if(newcontact || contact_el.hasClass("selected"))
        contact_el.click();
}

function newConversation() {
    $("#new-contact-modal .modal_close").click();
    $("#message-to-address").val($("#new-contact-address").val());
    $("#message-text").focus();
    $(".contact-discussion ul").html("<li id='remove-on-send'>Starting Conversation with "+$("#new-contact-address").val()+" - "+$("#new-contact-name").val()+"</li>");

    $("#new-contact-address").val("");
    $("#new-contact-name").val("");
    $("#new-contact-pubkey").val("");
    $("#contact-list ul li").removeClass("selected");
    $("#contact-list").addClass("in-conversation");
}


function sendMessage() {
    $("#remove-on-send").remove();
    if(bridge.sendMessage($("#message-to-address").val(), $("#message-text").val(), $("#message-from-address").val()))
        $("#message-text").val("");
}

function deleteMessages(key, messageid) {
    var contact = contacts[key];

    if(!confirm("Are you sure you want to delete " + (messageid == undefined ? 'these messages?' : 'this message?')))
        return false;

    var message_count = $("#message-count"),
        message_count_val = parseInt(message_count.text());

    for(var i=0;i<contact.messages.length;i++) {

        if(messageid == undefined) {
            if(bridge.deleteMessage(contact.messages[i].id))
            {
                $("#"+contact.messages[i].id).remove();

                if(contact.messages[i].type=="R" && contact.messages[i].read == false)
                {
                    message_count_val--
                    message_count.text(message_count_val);
                    if(message_count_val==0)
                        message_count.hide();
                    else
                        message_count.show();
                }

                contact.messages.splice(i, 1);
                i--;
            }
            else
                return false;
        }
        else
            if(contact.messages[i].id == messageid)
                if(bridge.deleteMessage(messageid)) {
                    $("#"+messageid).remove();

                    if(contact.messages[i].type=="R" && contact.messages[i].read == false)
                    {
                        message_count_val--
                        message_count.text(message_count_val);
                        if(message_count_val==0)
                            message_count.hide();
                        else
                            message_count.show();
                    }

                    contact.messages.splice(i, 1);
                    i--;
                    var notifications = $("#contact-"+ key).find(".message-notifications");
                    notifications.text(parseInt(notifications.text())-1);
                    break;
                }
                else
                    return false;
    }

    if(contact.messages.length == 0)
    {
        $("#contact-"+ key).remove();
        $("#contact-list").removeClass("in-conversation");
    }
    else
        iscrollReload();
}


var contactScroll = new IScroll('#contact-list', {
    mouseWheel: true,
    scrollbars: true,
    lockDirection: true,
    interactiveScrollbars: true,
    scrollbars: 'custom',
    scrollY: true,
    scrollX: false,
    preventDefaultException:{ tagName: /^(INPUT|TEXTAREA|BUTTON|SELECT|P|SPAN)$/ }
});

var messagesScroller = new IScroll('.contact-discussion', {
   mouseWheel: true,
   scrollbars: true,
   lockDirection: true,
   interactiveScrollbars: true,
   scrollbars: 'custom',
   scrollY: true,
   scrollX: false,
   preventDefaultException:{ tagName: /^(INPUT|TEXTAREA|BUTTON|SELECT|P|SPAN)$/ }
});

function iscrollReload(scroll) {
    contactScroll.refresh();
    messagesScroller.refresh();

    if(scroll == true)
        messagesScroller.scrollTo(0, messagesScroller.maxScrollY, 0);
}


function editorCommand(text, endText) {

        var range, start, end, txtLen, scrollTop;

        var editor = $("#message-text")[0];

        scrollTop = editor.scrollTop;
        editor.focus();


        if (typeof editor.selectionStart !== 'undefined')
        {
                start  = editor.selectionStart;
                end    = editor.selectionEnd;
                txtLen = text.length;

                if(endText)
                        text += editor.value.substring(start, end) + endText;

                editor.value = editor.value.substring(0, start) + text + editor.value.substring(end, editor.value.length);

                editor.selectionStart = (start + text.length) - (endText ? endText.length : 0);
                editor.selectionEnd = editor.selectionStart;
        }
        else
            editor.value += text + endText;

        editor.scrollTop = scrollTop;
        editor.focus();
};


var chainDataPage = {
    anonOutputs: {},
    init: function() {
        $("#show-own-outputs,#show-all-outputs").on("click", function(e) {
            $(e.target).hide().siblings('a').show();
        });

        $("#show-own-outputs").on("click", function() {
            $("#chaindata .footable tbody tr>td:first-child+td").each(function() {
                if($(this).text()==0)
                    $(this).parents("tr").hide();
            });
        });

        $("#show-all-outputs").on("click", function() {
            $("#chaindata .footable tbody tr:hidden").show();
        });
    },
    updateAnonOutputs: function() {
        chainDataPage.anonOutputs = bridge.listAnonOutputs();
        var tbody = $('#chaindata .footable tbody');
        tbody.html('');

        for (value in chainDataPage.anonOutputs) {
            var anonOutput = chainDataPage.anonOutputs[value];

            tbody.append('<tr>\
                    <td data-value='+value+'>'+anonOutput.value_s+'</td>\
                    <td>' +  anonOutput.owned_outputs
                          + (anonOutput.owned_outputs == anonOutput.owned_mature
                            ? ''
                            : ' (<b>' + anonOutput.owned_mature + '</b>)') + '</td>\
                    <td>'+anonOutput.system_outputs + ' (' + anonOutput.system_mature + ')</td>\
                    <td>'+anonOutput.system_spends  +'</td>\
                    <td>'+anonOutput.least_depth    +'</td>\
                </tr>');
        }

        $('#chaindata .footable').trigger('footable_initialize');
    }
}
var blockExplorerPage = 
{
    init: function() {},
    blockHeader: {},
    findBlock: function(searchID) {

        if(searchID == "" || searchID == null)
        {
            blockExplorerPage.updateLatestBlocks();
        }
        else
        {
            blockExplorerPage.foundBlock = bridge.findBlock(searchID);

            if(blockExplorerPage.foundBlock.error_msg != '' )
            { 
                $('#latest-blocks-table  > tbody').html('');
                $("#block-txs-table > tbody").html('');
                $("#block-txs-table").addClass("none");
                alert(blockExplorerPage.foundBlock.error_msg);
                return false;
            } 

            var tbody = $('#latest-blocks-table  > tbody');
            tbody.html('');
            var txnTable = $('#block-txs-table  > tbody');
            txnTable.html('');
            $("#block-txs-table").addClass("none");

            tbody.append('<tr data-value='+blockExplorerPage.foundBlock.block_hash+'>\
                                     <td>'+blockExplorerPage.foundBlock.block_hash+'</td>\
                                     <td>'+blockExplorerPage.foundBlock.block_height+'</td>\
                                     <td>'+blockExplorerPage.foundBlock.block_timestamp+'</td>\
                                     <td>'+blockExplorerPage.foundBlock.block_transactions+'</td>\
                        </tr>'); 
            blockExplorerPage.prepareBlockTable();
        }
        // Keeping this just in case - Will remove if not used 
    },
    updateLatestBlocks: function() 
    {
        blockExplorerPage.latestBlocks = bridge.listLatestBlocks();
        var txnTable = $('#block-txs-table  > tbody');
        txnTable.html('');
        $("#block-txs-table").addClass("none");
        var tbody = $('#latest-blocks-table  > tbody');
        tbody.html('');
        for (value in blockExplorerPage.latestBlocks) {

            var latestBlock = blockExplorerPage.latestBlocks[value];

            tbody.append('<tr data-value='+latestBlock.block_hash+'>\
                         <td>' +  latestBlock.block_hash   + '</td>\
                         <td>' +  latestBlock.block_height + '</td>\
                         <td>' +  latestBlock.block_timestamp   + '</td>\
                         <td>' +  latestBlock.block_transactions+ '</td>\
                         </tr>'); 
        }
        blockExplorerPage.prepareBlockTable();
    },
    prepareBlockTable: function()
    {
        $("#latest-blocks-table  > tbody tr")
            .on('click', function()
                { 
                    $(this).addClass("selected").siblings("tr").removeClass("selected"); 
                    var blkHash = $(this).attr("data-value").trim();
                    blockExplorerPage.blkTxns = bridge.listTransactionsForBlock(blkHash);
                    var txnTable = $('#block-txs-table  > tbody');
                    txnTable.html('');
                    for (value in blockExplorerPage.blkTxns)
                    {
                        var blkTx = blockExplorerPage.blkTxns[value];

                        txnTable.append('<tr data-value='+blkTx.transaction_hash+'>\
                                    <td>' +  blkTx.transaction_hash  + '</td>\
                                    <td>' +  blkTx.transaction_value + '</td>\
                                    </tr>'); 
                    }
                    $("#block-txs-table").removeClass("none");

                    $("#block-txs-table > tbody tr")
                        .on('click', function() {
                            $(this).addClass("selected").siblings("tr").removeClass("selected");
                        })

                        .on("dblclick", function(e) {
                            $(this).attr("href", "#blkexp-txn-modal");

                            $(this).leanModal({ top : 10, overlay : 0.5, closeButton: "#blkexp-txn-modal .modal_close" });

                            selectedTxn = bridge.txnDetails(blkHash , $(this).attr("data-value").trim());

                            if(selectedTxn.error_msg == '')
                            {
                                $("#txn-hash").html(selectedTxn.transaction_hash);
                                $("#txn-size").html(selectedTxn.transaction_size);
                                $("#txn-rcvtime").html(selectedTxn.transaction_rcv_time);
                                $("#txn-minetime").html(selectedTxn.transaction_mined_time);
                                $("#txn-blkhash").html(selectedTxn.transaction_block_hash);
                                $("#txn-reward").html(selectedTxn.transaction_reward);
                                $("#txn-confirmations").html(selectedTxn.transaction_confirmations);
                                $("#txn-value").html(selectedTxn.transaction_value);            
                                $("#error-msg").html(selectedTxn.error_msg);

                                if(selectedTxn.transaction_reward > 0)
                                {
                                    $("#lbl-reward-or-fee").html('<strong>Reward</strong>');
                                    $("#txn-reward").html(selectedTxn.transaction_reward);
                                }
                                else
                                {
                                    $("#lbl-reward-or-fee").html('<strong>Fee</strong>');
                                    $("#txn-reward").html(selectedTxn.transaction_reward * -1);
                                }
                            }
                            
                            var txnInputs = $('#txn-detail-inputs > tbody');
                            txnInputs.html('');
                            for (value in selectedTxn.transaction_inputs) {

                              
                              
                              var txnInput = selectedTxn.transaction_inputs[value];

                              txnInputs.append('<tr data-value='+ txnInput.input_source_address+'>\
                                                           <td>' + txnInput.input_source_address  + '</td>\
                                                           <td>' + txnInput.input_value + '</td>\
                                                </tr>'); 
                            }

                            var txnOutputs = $('#txn-detail-outputs > tbody');
                            txnOutputs.html('');

                            for (value in selectedTxn.transaction_outputs) {

                              var txnOutput = selectedTxn.transaction_outputs[value];

                              txnOutputs.append('<tr data-value='+ txnOutput.output_source_address+'>\
                                                 <td>' +  txnOutput.output_source_address  + '</td>\
                                                 <td>' +  txnOutput.output_value + '</td>\
                                            </tr>'); 
                            }


                           
                            $(this).click();
                            $(this).off('click');
                            $(this).on('click', function() {
                                    $(this).addClass("selected").siblings("tr").removeClass("selected");
                            })
                        }).find(".editable")
                })
            .on("dblclick", function(e) 
            {
                $(this).attr("href", "#block-info-modal");

                $(this).leanModal({ top : 10, overlay : 0.5, closeButton: "#block-info-modal .modal_close" });
                
                selectedBlock = bridge.blockDetails($(this).attr("data-value").trim()) ; 

                if(selectedBlock)
                {
                     $("#blk-hash").html(selectedBlock.block_hash);
                     $("#blk-numtx").html(selectedBlock.block_transactions);
                     $("#blk-height").html(selectedBlock.block_height);
                     $("#blk-type").html(selectedBlock.block_type);
                     $("#blk-reward").html(selectedBlock.block_reward);
                     $("#blk-timestamp").html(selectedBlock.block_timestamp);
                     $("#blk-merkleroot").html(selectedBlock.block_merkle_root);
                     $("#blk-prevblock").html(selectedBlock.block_prev_block);
                     $("#blk-nextblock").html(selectedBlock.block_next_block);
                     $("#blk-difficulty").html(selectedBlock.block_difficulty);
                     $("#blk-bits").html(selectedBlock.block_bits);
                     $("#blk-size").html(selectedBlock.block_size);
                     $("#blk-version").html(selectedBlock.block_version);
                     $("#blk-nonce").html(selectedBlock.block_nonce);
                }

                // $("#block-info").html();
                $(this).click();

                $(this).off('click');
                $(this).on('click', function() {
                $(this).addClass("selected").siblings("tr").removeClass("selected");
                })
            }).find(".editable")
    }
}
