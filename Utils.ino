/* 
 * Converts Centimeters to Meters
 * 
 * returns: float meters
 */
float cm_to_m(float cm){
    return cm / 100;
}


/* Converts String to Char* */
char* string2char(String s){
  if(s.length()!=0){
      char *p = const_cast<char*>(s.c_str());
      return p;
  }
}


/* Checks if the given string is a number.
    - ToDo: Return Fakse for "a1" (alpha-numeric) strings.
*/
boolean isANumber(String s){
    for(byte i=0; i<s.length(); i++){
      if(isDigit(s.charAt(i))){
        // If it's a numeric:
        return true;
      }
    }
    // if it cotains any letter:
    return false;
}
