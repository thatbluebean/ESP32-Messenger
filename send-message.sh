MSG="$*"
curl -X POST -d "msg=$MSG" http://hostserver/set
