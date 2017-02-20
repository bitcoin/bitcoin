
function CheckboxMenu(id, data, persistkeys, globals)
{
    this.id = id;
    this.menuCheckboxIds = new Array();
    this.data = data;
    this.count = 0;
    
    var element = document.getElementById(id);
    var checkboxNodes = element.getElementsByTagName("input");

    for(var checkboxCount=0; checkboxCount < checkboxNodes.length; checkboxCount++)
    {
        var checkboxId = checkboxNodes[checkboxCount].getAttribute('id');
        var checkboxData = checkboxNodes[checkboxCount].getAttribute('data');
        var dataSplits = checkboxData.split(',');
        var defaultValue = checkboxNodes[checkboxCount].getAttribute('value');
        if (checkboxData != null && checkboxData.indexOf("persist") != -1)
            persistkeys.push(checkboxId);
        
        this.menuCheckboxIds[dataSplits[0]] = checkboxId;
        
        // try to get the value for this checkbox id from globals
        var persistedValue = (globals == null) ? null : globals.VariableExists(checkboxId) ? globals.VariableValue(checkboxId) : null;
        var currentValue = (persistedValue != null) ? persistedValue : (defaultValue == null) ? "on" : defaultValue;
        
        // set the checkbox's check state
        this.SetCheckState(checkboxId, currentValue);
        
        this.count++;
    }
}

CheckboxMenu.prototype.SetCheckState=function(id, value)
{
    var checkbox = document.getElementById(id);
    if(checkbox != null)
    {
        checkbox.checked = (value == "on") ? true : false;
    }
    
    // set the value for the checkbox id in the data array
    this.data[id] = value;
}

CheckboxMenu.prototype.GetCheckState=function(id)
{
    var checkbox = document.getElementById(id);
    if(checkbox != null)
        return checkbox.checked;
    return false;
}

CheckboxMenu.prototype.ToggleCheckState=function(id)
{
    // at least one checkbox must always be checked
    var checkedCount = this.GetCheckedCount();
        
    if(this.data[id] == "on" && checkedCount > 1)
        this.SetCheckState(id, "off");
    else
        this.SetCheckState(id, "on");
}

// returns the checkbox id associated with a key
CheckboxMenu.prototype.GetCheckboxId=function(key)
{
    return this.menuCheckboxIds[key];
}

// returns the array of checkbox ids
CheckboxMenu.prototype.GetCheckboxIds=function()
{
    return this.menuCheckboxIds;
}

// returns the @data attribute of the checkbox element
CheckboxMenu.prototype.GetCheckboxData=function(checkboxId)
{
    var checkbox = document.getElementById(checkboxId);
    if (checkbox == null) return "";
    return checkbox.getAttribute('data');
}

CheckboxMenu.prototype.GetDropdownLabelId=function()
{
    var checkboxCount = this.count;
    var checkedCount = this.GetCheckedCount();
    var idPrefix = this.id;
    
    // if all boxes checked, use showall label
    if (checkedCount == checkboxCount)
        return idPrefix.concat("AllLabel");
    
    // if only one is checked, use label appropriate for that one checkbox
    if (checkedCount == 1)
    {
        for(var key in this.menuCheckboxIds)
        {
            if (this.data[this.menuCheckboxIds[key]] == "on")
            {
                return idPrefix.concat(key,'Label');
            }
        }
    }
    
    // if multiple or zero checked, use multiple label
    return idPrefix.concat("MultipleLabel");
}

CheckboxMenu.prototype.GetCheckedCount=function()
{
    var count = 0;
    for(var key in this.menuCheckboxIds)
    {
        if (this.data[this.menuCheckboxIds[key]] == "on")
            count++;
    }
    return (count);
}

// returns an array containing the ids of the checkboxes that are checked
CheckboxMenu.prototype.GetCheckedIds=function()
{
    var idArray = new Array();
    for(var key in this.menuCheckboxIds)
    {
        if (this.data[this.menuCheckboxIds[key]] == "on")
            idArray.push(this.menuCheckboxIds[key]);
    }
    return idArray;
}

CheckboxMenu.prototype.GetGroupCheckedCount=function(checkboxGroup)
{
    var count = 0;
    for(var i = 0; i < checkboxGroup.length; i++)
    {
        if (this.data[checkboxGroup[i]] == "on")
            count++;
    }
    return (count);
}

CheckboxMenu.prototype.ToggleGroupCheckState=function(id, checkboxGroup)
{
    // at least one checkbox must always be checked
    var checkedCount = this.GetGroupCheckedCount(checkboxGroup);
    
    // if the group has multiple checkboxes, one must always be checked; so toggle to "off" only if more than one currently checked
    // if the group has only one checkbox, it's okay to toggle it on/off
    if(this.data[id] == "on" && (checkedCount > 1 || checkboxGroup.length == 1))
        this.SetCheckState(id, "off");
    else
        this.SetCheckState(id, "on");
}

