
// Dropdown menu control

function Dropdown(activatorId, dropdownId) {

    // store activator and dropdown elements
    this.activator = document.getElementById(activatorId);
    this.dropdown = document.getElementById(dropdownId);
    this.activatorImage = document.getElementById(activatorId + "Image");

    // wire up show/hide events
    registerEventHandler(this.activator,'mouseover', getInstanceDelegate(this, "show"));
    registerEventHandler(this.activator,'mouseout', getInstanceDelegate(this, "requestHide"));
    registerEventHandler(this.dropdown,'mouseover', getInstanceDelegate(this, "show"));
    registerEventHandler(this.dropdown,'mouseout', getInstanceDelegate(this, "requestHide"));

    // fix visibility and position
    this.dropdown.style.visibility = 'hidden';
    this.dropdown.style.position = 'absolute';
    this.reposition(null);

    // wire up repositioning event
    registerEventHandler(window, 'resize', getInstanceDelegate(this, "reposition"));


}

Dropdown.prototype.show = function(e) {
    clearTimeout(this.timer);
    this.dropdown.style.visibility = 'visible';
    if (this.activatorImage != null)
        this.activatorImage.src = dropDownHoverImage.src;
    if (this.activator != null)
        this.activator.className = "filterOnHover";
}

Dropdown.prototype.hide = function(e) {
    this.dropdown.style.visibility = 'hidden';
    if (this.activatorImage != null)
        this.activatorImage.src = dropDownImage.src;
    if (this.activator != null)
        this.activator.className = "filter";
}

Dropdown.prototype.requestHide = function(e) {
    this.timer = setTimeout( getInstanceDelegate(this, "hide"), 250);
}

Dropdown.prototype.reposition = function(e) {

    // get position of activator
    var offsetLeft = 0;
    var offsetTop = 0;
    var offsetElement = this.activator;
    
    while (offsetElement) {
        offsetLeft += offsetElement.offsetLeft;
        offsetTop += offsetElement.offsetTop;
        offsetElement = offsetElement.offsetParent;
    }

    // set position of dropdown relative to it
    this.dropdown.style.left = offsetLeft;
    this.dropdown.style.top = offsetTop + this.activator.offsetHeight;
}

Dropdown.prototype.SetActivatorLabel = function(labelId)
{
    // get the children of the activator node, which includes the label nodes
    var labelNodes = this.activator.childNodes;


    for(var labelCount=0; labelCount < labelNodes.length; labelCount++)
    {
        if(labelNodes[labelCount].tagName == 'LABEL')
        {
            var labelNodeId = labelNodes[labelCount].getAttribute('id');
            if (labelNodeId == labelId)
            {
                labelNodes[labelCount].style.display = "inline";
            }
            else
            {
                labelNodes[labelCount].style.display = "none";
            }
        }
    }
}
 