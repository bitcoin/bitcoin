case "$OSTYPE" in
  linux*)   spd-say "Welcome to the oh cho. All eights, all the time." ;;
  darwin*)  say Welcome to the oh cho. All eights, all the time ;; 
  msys*)    echo "windows" ;;
  solaris*) echo "solaris" ;;
  bsd*)     echo "bsd" ;;
  *)        echo "unknown" ;;
esac
