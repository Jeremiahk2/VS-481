// 
// A gentle introduction, using the print callback registered in the native
// main() function
//

//This better be the event
var death = moreArgs();
print(death.type);
//Set it up.
death.type = "spawn";
death.time = death.time + 3000;
death.order = death.order + 1;
//Raise the new event
raise(death.guid);