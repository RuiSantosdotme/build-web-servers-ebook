// Get current sensor readings when the page loads  
window.addEventListener('load', getValues);

// Function to get current readings on the webpage when it loads/refreshes
function getValues(){
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var myObj = JSON.parse(this.responseText);
      console.log(myObj);
      document.getElementById("textFieldValue").innerHTML = myObj.textValue;
      document.getElementById("numberFieldValue").innerHTML = myObj.numberValue;
    }
  }; 
  xhr.open("GET", "/values", true);
  xhr.send();
}