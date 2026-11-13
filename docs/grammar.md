# گرامر B-Minor

در این EBNF، متن داخل کوتیشن یک توکن مشخص است، `|` انتخاب، `[]` بخش اختیاری و `{}` تکرار صفر یا بیشتر را نشان می‌دهد. `EOF` پایان ورودی است. توضیحات و فاصله‌های سفید پیش از تحلیل نحوی کنار گذاشته می‌شوند.

```ebnf
program       = { global-decl }, EOF ;
global-decl   = identifier, ":", ( function-type, ( ";" | "=", block )
                                | value-type, [ "=", initializer ], ";" ) ;
local-decl    = identifier, ":", ( function-type, ";"
                                | value-type, [ "=", initializer ], ";" ) ;
atomic        = "integer" | "boolean" | "char" | "string" ;
return-type   = atomic | "void" ;
function-type = "function", return-type, "(", [ parameters ], ")" ;
parameters    = parameter, { ",", parameter } ;
parameter     = identifier, ":", parameter-type ;
parameter-type = atomic | function-type
               | "array", "[", [ expression ], "]", parameter-type ;
value-type    = atomic | "array", "[", expression, "]", element-type ;
element-type  = value-type | function-type ;
initializer   = expression | "{", [ initializer, { ",", initializer } ], "}" ;

block         = "{", { statement }, "}" ;
statement     = block | local-decl | if-stmt | for-stmt
              | "return", [ expression ], ";"
              | "print", [ arguments ], ";"
              | [ expression ], ";" ;
if-stmt       = "if", "(", expression, ")", statement, [ "else", statement ] ;
for-stmt      = "for", "(", [ expression ], ";", [ expression ], ";",
               [ expression ], ")", statement ;
arguments     = expression, { ",", expression } ;

expression    = assignment ;
assignment    = logical-or, [ "=", assignment ] ;
logical-or    = logical-and, { "||", logical-and } ;
logical-and   = equality, { "&&", equality } ;
equality      = relational, { ( "==" | "!=" ), relational } ;
relational    = additive, { ( "<" | "<=" | ">" | ">=" ), additive } ;
additive      = multiplicative, { ( "+" | "-" ), multiplicative } ;
multiplicative = power, { ( "*" | "/" | "%" ), power } ;
power         = unary, [ "^", power ] ;
unary         = ( "-" | "!" ), unary | postfix ;
postfix       = primary, { "(", [ arguments ], ")" | "[", expression, "]" },
               [ "++" | "--" ] ;
primary       = identifier | integer-literal | boolean-literal
              | string-literal | character-literal | "(", expression, ")" ;
```

## روش تحلیل

Parser بازگشتی نزولی است. در سطح دستور، دو توکن نگاه به جلو کافی است: `identifier :` اعلان است و شناسه در حالت دیگر شروع عبارت است. اعلان‌ها بعد از پیشوند مشترک `identifier :` بر اساس نوع جدا می‌شوند. برای عبارت‌ها از precedence climbing استفاده می‌شود؛ این روش همان لایه‌های تقدم بالا را با یک تابع و جدول تقدم پیاده می‌کند.

بازگشت چپ وجود ندارد: قواعد جمع، ضرب و مقایسه از تکرار استفاده می‌کنند. بازگشت در انتساب و توان در سمت راست است. برای مثال `a-b-c` برابر `(a-b)-c` و `a=b=c` برابر `a=(b=c)` است. توان نیز راست‌گرا انتخاب شده است: `2^3^2` برابر `2^(3^2)` است. منفی یکانی پیش از توان قرار دارد، پس `-2^2` به صورت `(-2)^2` تحلیل می‌شود.

در جدول توصیفی زبان، مقایسه‌ها و عملگرهای منطقی گروه‌بندی شده‌اند؛ این پیاده‌سازی با توجه به شباهت ذکرشده به C، تقدم `< <= > >=` را بالاتر از `== !=` و تقدم `&&` را بالاتر از `||` در نظر می‌گیرد. راست‌گرایی توان، دستور خالی، `print;` و فهرست مقداردهی خالی قراردادهای صریح این گرامر هستند. تطبیق نوع، قابل‌انتساب‌بودن سمت چپ و معتبر بودن نوع عناصر آرایه به تحلیل معنایی تعلق دارند.

## تصمیم‌های FIRST و FOLLOW

`E` را مجموعهٔ شروع عبارت می‌گیریم: شناسه، چهار دستهٔ ثابت، `(`، `-` و `!`.

| قاعده یا انتخاب | FIRST / نگاه به جلو | پایان یا FOLLOW مرتبط |
|---|---|---|
| اعلان | شناسه و سپس `:` | اعلان بعدی، شروع دستور، `}` یا EOF |
| نوع | `integer boolean char string array function` | `= ; , )` یا شروع پارامترهای تابع |
| عبارت | E | `; , ) ] }` |
| آرگومان اختیاری | E در برابر `)` | `)` |
| مقداردهی اولیه | E در برابر `{` | `, } ;` |
| عبارت اختیاری for | E در برابر `;` یا `)` | جداکنندهٔ همان بخش |
| ادامهٔ جمع | `+ -` | عملگر ضعیف‌تر یا پایان عبارت |
| ادامهٔ ضرب | `* / %` | عملگر ضعیف‌تر یا پایان عبارت |
| ادامهٔ پسوند | `( [` و سپس `++ --` اختیاری | عملگر دودویی یا پایان عبارت |

این جدول تصمیم‌های مورد استفاده را نشان می‌دهد، نه ادعای LL(1) بودن کل گرامر. دو اشتراک مهم داریم: اعلان و عبارت با شناسه شروع می‌شوند که با دو توکن حل می‌شود؛ در `if-stmt` نیز `else` هم در FIRST بخش اختیاری و هم در FOLLOW دستور داخلی دیده می‌شود. برای رفع dangling else، Parser همیشه `else` را به نزدیک‌ترین `if` بدون `else` متصل می‌کند. در نتیجه گرامر نوشتاری با این قرارداد اجرایی تفسیر می‌شود.

## AST و بازیابی خطا

برگ‌ها `Identifier` و `Literal` هستند. گره‌های داخلی از جمله `VarDecl`، `FuncDecl`، `Type`، `ParamDecl`، `BlockStmt`، `IfStmt`، `ForStmt`، `ReturnStmt`، `PrintStmt`، `BinaryExpr`، `UnaryExpr`، `PostfixExpr`، `IndexExpr` و `CallExpr` هستند. `GroupExpr` محل پرانتز آغازین را نگه می‌دارد. `Omitted` جای بخش حذف‌شدهٔ for یا اندازهٔ نامشخص پارامتر آرایه را حفظ می‌کند.

هر گره توکن اصلی، موقعیت اولین توکن سازه، فرزندان و فیلد خالی `inferredType` دارد. در عبارت دودویی، `token` خود عملگر است ولی `location` ابتدای عملوند چپ است. فرزندان به ترتیب گرامر نگهداری می‌شوند. برای تابع، فرزند اول نوع و فرزند دوم، در صورت وجود، بدنه است. برای فراخوانی، فرزند اول مقصد و بقیه آرگومان‌ها هستند. for سه فرزند برای بخش‌های سرآیند و یک فرزند برای بدنه دارد.

در Panic Mode، خطا در نزدیک‌ترین بلوک یا سطح برنامه گرفته می‌شود. یک گره `Error` ثبت می‌شود و پیمایش تا `;`، `}`، اعلان بعدی یا شروع `if/for/return/print` ادامه می‌یابد. عمق آکولادهای ردشده حساب می‌شود تا بدنهٔ خراب با اعلان بعدی اشتباه نشود. هر چرخه یا توکن مصرف می‌کند یا کنترل را به مالک `}` بازمی‌گرداند؛ EOF نیز همیشه پایان تحلیل است.

برای جلوگیری از سرریز پشته، عمق فراخوانی‌های محافظت‌شدهٔ Parser به ۱۲۸ و ارتفاع AST به ۲۵۶ محدود است. این اعداد محدودیت اجرایی‌اند، نه قاعدهٔ زبان؛ تجاوز از آن‌ها تشخیص داده و گزارش می‌شود.

`while` فقط رزروشده است و تولید نحوی ندارد. تعریف تابع داخل تابع مجاز نیست. بررسی وجود main، حوزه‌ها، نوع عبارات، اندازهٔ ثابت و مثبت آرایه، بازهٔ عدد ۶۴بیتی و ثابت‌بودن مقداردهی سراسری جزو تحلیل معنایی فاز بعد است.
