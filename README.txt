Linor Ben Yossef  207331182
Keren Rachev 318638129

*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	דינאמי יותר טוב כשיש רשימה ארוכה יותר לעומת הסטטי שבו צריך לשלוח לכל סוכן את כל המידע הרלוונטי לאחסון ועיבוד.//
//	כאשר נכנסים ליותר מידע האחסון הנדרש הוא יותר גדול שכן לכל סוכן לא ייקח זמן זהה לסיים כמות שווה של חישובים.//
//	כל חישוב לוקח זמן שונה ותלוי במספרים ולכל סוכן יש משאבי ריצה שונים (תלוי מערכת והניהול הפנימי שלה).//
//	בתוכנית דינאמית הסוכנים שמסיימים יותר מהר יקבלו יותר עבודה.//
// נשים לב בתוצאות שלמספר תהליכים גדול יש בהרצה הדינמית זמן ריצה משופר ביחס לסטטי //
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

Static_gcd:
mpiexec -n 3 ./test 
The time it took to calculate is 0.000087 seconds
mpiexec -n 10 ./test
The time it took to calculate is 0.034558 seconds
mpiexec -n 5 ./test 
The time it took to calculate is 0.013197 seconds

Dynamic_gcd:
mpiexec -n 3 ./test 50
The time it took to calculate is 0.001157 seconds
mpiexec -n 3 ./test 100
The time it took to calculate is 0.001234 seconds

mpiexec -n 10 ./test 100 // CHUNK SIZE גדול מידי לחלוקת עבודה יעילה בין התהליכים
The time it took to calculate is 0.069032 seconds
mpiexec -n 10 ./test 10
The time it took to calculate is 0.025931 seconds
mpiexec -n 5 ./test 25
The time it took to calculate is 0.002292 seconds
