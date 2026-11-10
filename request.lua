local method = os.getenv("METHOD") or "ECHO"

if method == "GET" then
    wrk.method = "GET"
    wrk.path = "/users"

elseif method == "POST" then
    wrk.method = "POST"
    wrk.path = "/print"
    wrk.headers["Content-Type"] = "text/plain"
    wrk.body = "Hello World"

else
    wrk.method = "GET"
    wrk.path = "/"
end