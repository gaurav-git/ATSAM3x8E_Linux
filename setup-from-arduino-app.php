#!/usr/bin/php
<?
// Short functions
define("CHK_ROOT_PATH", rtrim(__DIR__, '/'));
function chk($f) { if(realpath(dirname($f))===false || stripos(realpath(dirname($f)), CHK_ROOT_PATH) === false) _yield("Path '$f' is not in the allowed base path."); }
function _yield($text) { file_put_contents("php://stderr", "\e[31m$text\e[0m\n"); exit(1); }
function e($arg) { return escapeshellarg($arg); }
function sh($cmd) { @system($cmd, $r); return $r; }
function gsh($cmd) { ob_start(); @system($cmd); return @ob_get_clean(); }
function cp($from, $to) { chk(($to)); if(sh("cp -R " . e($from) . " " . e($to))) _yield("Failed to copy '$from' --> '$to'"); }
function cd($to) { if(!chdir($to)) _yield("Failed to change directory to '$to'"); }
function mv($from, $to) { chk(($to)); if(sh("mv ".e($from)." ".e($to))) _yield("Failed to move '$from' --> '$to'"); }
function rm($f) { chk(($f)); if(!($f=realpath($f)) || sh("rm -rf " .e($f) != 0)) _yield("Failed to remove $f");  } ;
function md($dir) { chk(($dir)); if(sh("mkdir -p " . e($dir))) _yield("Failed create directory '$dir'"); }
function arg($i) { return isset($_SERVER['argv'][$i]) ? $_SERVER['argv'][$i] : null; }
function pwd() { return getcwd(); }
function gfile($f) { if(($t=file_get_contents($f))===false) _yield("Failed to read file '$f'"); return $t; }
function wfile($f, $t) { chk(($f)); if(!file_put_contents($f, $t)) _yield("Failed to write file '$f'"); }
function cat($f) { print gfile($f) . "\n"; }
function repf($f, $s, $r, $sim=false) { chk(($f)); $t=preg_replace($s, $r, gfile($f)); if(!$ret) wfile($f, $t); return $t; }
function srchf($f, $s, &$m=null) {  return preg_match($s, gfile($f), $m); }
function relpath($r, $f) { if($r==$f) return "."; while(strlen($r) && strlen($f) && substr($r,0,1)===substr($f,0,1)) { $r=substr($r,1); $f=substr($f, 1); } return trim(!strlen($r) ? '.' : str_repeat('../',  count(explode('/', $r))) . $f, '/'); }

// Config
$ROOT_DIR = rtrim(__DIR__, "/") . "/due";
$SCRIPT_RESOURCE_DIR = __DIR__ . "/resources";
$TOOLS_DIR = "$ROOT_DIR/tools";
$GCC_EABI_DIR = "$TOOLS_DIR/../../../tools/arm-none-eabi-gcc/4.8.3-2014q1/";
$BOSSAC = "$TOOLS_DIR/bossac";
$HARDW_DIR = "$ROOT_DIR/sam";
$DIST_LIB_DIR = "$ROOT_DIR/lib";
$DIST_INC_DIR = "$ROOT_DIR/include";
$DIST_SRC_DIR = "$ROOT_DIR/src";
print "ROOTDIR: $ROOT_DIR\n";
print "TOOLSDIR: $TOOLS_DIR\n";
// Run
if(!is_dir(arg(1))) _yield('You need to specify your Arduino directory. It contains the folders examples, hardware, ...');

cd(arg(1));
$SRC_DIR = pwd();
md($ROOT_DIR);
md($TOOLS_DIR);
md($HARDW_DIR);
md($DIST_LIB_DIR);

print "GCC ARM EABI toolchain ...\n";
md("$TOOLS_DIR/arm-none-eabi-gcc");
cp("tools/arm-none-eabi-gcc", "$TOOLS_DIR/");

print "BOSSA uploader/downloader ...\n";
cp("tools/bossac", $BOSSAC);

print "Arduino & Atmel libraries (SAM, CMSIS) ...\n";

cp("hardware/sam/1.6.12/system/CMSIS", "$HARDW_DIR/");
cp("hardware/sam/1.6.12/system/libsam", "$HARDW_DIR/");
cp("hardware/sam/1.6.12/variants/arduino_due_x/linker_scripts", "$HARDW_DIR/");

cd($HARDW_DIR);
print "Copying existing libraries ... \n";
sh("find . -type f -exec chmod 644 {} \\;");
sh("find . -type d -exec chmod 755 {} \\;");
sh("find . -type f -name *.a -exec cp {} '$DIST_LIB_DIR' \\;");

print "Copying 'patch' resources \n";
system("cp -R " . e("$SCRIPT_RESOURCE_DIR/include") . "* " . e("$DIST_INC_DIR") . " >/dev/null 2>&1");
system("cp -R " . e("$SCRIPT_RESOURCE_DIR/make") . "* " . e("$ROOT_DIR/make")  . " >/dev/null 2>&1");
system("cp -R " . e("$SCRIPT_RESOURCE_DIR/example") . "* " . e("$ROOT_DIR/example")  . " >/dev/null 2>&1");

print "Creating libsam ... \n";
cd("$HARDW_DIR/libsam/build_gcc");
repf("Makefile", "|(OUTPUT_BIN)[\\s]*=[\\s]*[^\\s]+|ismx", "\\1=" . relpath(pwd(), $DIST_LIB_DIR));
repf("sam3.mk", "|(OUTPUT_BIN)[\\s]*=[\\s]*[^\\s]+|ismx", "\\1=" . relpath(pwd(), $DIST_LIB_DIR));

if(!srchf("gcc.mk", "|\\nARM_GCC_TOOLCHAIN=|i")) {
  repf("gcc.mk", "|[\n]([\s]*CROSS_COMPILE[\s]*=)|ismx", "\nARM_GCC_TOOLCHAIN=" . relpath(pwd(), $TOOLS_DIR) . "/arm-none-eabi-gcc/4.8.3-2014q1/bin\n\\1");
}

if(($ec=sh("make clean >/dev/null"))!=0) _yield("Make returned exit code $ec.");
if(($ec=sh("make arduino_due_x >/dev/null"))!=0) _yield("Make returned exit code $ec.");

