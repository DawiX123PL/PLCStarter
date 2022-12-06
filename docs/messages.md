
# Messages



## sending unnown command


example message:
```json
{
    "Cmd": "random",
}
```

response
```json
{
    "Cmd": "random",
    "Result":"ERROR",
    "Msg": "Unnown Command"
}
```



---

## cmd = UNNOWN

message:
```json
{
    "Cmd": "UNNOWN",
}
```

response:
```json
{
    "Cmd": "UNNOWN",
    "Result":"OK",
    "Msg": "XD"
}
```


---

## cmd = PING

message:

```json
{
    "Cmd": "PING",
}
```

response:
```json
{
    "Cmd": "PING",
    "Result":"OK",
    "Msg": "Pong"
}
```

---

## cmd = APP_START

message:
```json
{
    "Cmd": "APP_START",
}
```

responses:
 - if OK
    ```json
    {
        "Cmd": "APP_START",
        "Result":"OK",
    }
    ```
 - if Error
    ```json
    {
        "Cmd": "APP_START",
        "Result":"ERROR",
        "Msg": "Cannot Start App"
    }
    ```


---

## cmd = APP_STOP

message:
```json
{
    "Cmd": "APP_STOP",
}
```

responses:
 - if OK
    ```json
    {
        "Cmd": "APP_STOP",
        "Result":"OK",
    }
    ```
 - if Error
    ```json
    {
        "Cmd": "APP_STOP",
        "Result":"ERROR",
        "Msg": "Cannot Stop App"
    }
    ```


---

## cmd = APP_RESUME
 This command is not implemented.

---

## cmd = APP_PAUSE
 This command is not implemented.

---

## cmd = APP_STATUS

message:
```json
{
    "Cmd": "APP_STATUS",
}
```

responses:
 - if OK:
     - App is running:
        ```json
        {
            "Cmd": "APP_STATUS",
            "Result":"OK",
            "Status": "RUNNING"
        }
        ```
     - App is stopped:
        ```json
        {
            "Cmd": "APP_STATUS",
            "Result":"OK",
            "Status": "STOPPED"
        }
        ```

 - if Error:
    ```json
    {
        "Cmd": "APP_STATUS",
        "Result":"ERROR",
        "Msg": "Cannot Check App Status"
    }
    ```

---

## cmd = APP_BUILD

message:

```json
{
    "Cmd": "APP_BUILD",
}
```


example result:
```json
{
    "Cmd": "APP_BUILD",
    "Result":"OK",
    "CompilationResult": [
        {
            {"File", "file1.cpp"},
            {"ExitCode", 0},
            {"ErrorMsg", ""}
        },
        {
            {"File", "file2.cpp"},
            {"ExitCode", 0},
            {"ErrorMsg", "...some warnings..."}
        },
        {
            {"File", "file3.cpp"},
            {"ExitCode", 1},
            {"ErrorMsg", "...error message..."}
        },
    ]
}
```


---

## cmd = FILE_LIST

message:
```json
{
    "Cmd": "FILE_LIST",
}
```

example result:
```json
{
    "Cmd": "APP_BUILD",
    "Result":"OK",
    "DirEntry":[
        {
            {"Name": "file1.cpp" }, 
            {"Date": 1670343419 },
            {"Type": "File" }
        },
        {
            {"Name": "folder1" }, 
            {"Date": 1670343410 },
            {"Type": "Dir" }
        },
        {
            {"Name": "folder1/file2.cpp" }, 
            {"Date": 1670343419 },
            {"Type": "File" }
        },
        {
            {"Name": "something" }, 
            {"Date": 1670343410 },
            {"Type": "Other" }
        }
    ]
}
```


---

## cmd = FILE_WRITE

example message:
```json
{
    "Cmd": "FILE_WRITE",
    "FileName": "file1.cpp",
    "Data": "576974616D2050616E61"
}
```


response:
 - if OK
    ```json
    {
        "Cmd": "FILE_WRITE",
        "Result":"OK",
    }
    ```
 - example error
    ```json
    {
        "Cmd": "FILE_WRITE",
        "Result":"ERROR",
        "Msg": "Cound not write file"
    }
    ```



---

## cmd = FILE_READ

example message:
```json
{
    "Cmd": "FILE_READ",
    "FileName": "file1.cpp",
}
```

response:
 - if OK, example
    ```json
    {
        "Cmd": "FILE_READ",
        "Result":"OK",
        "Data": "576974616D2050616E61"
    }
    ```
 - example error
    ```json
    {
        "Cmd": "FILE_READ",
        "Result":"ERROR",
        "Msg": "Cannot read file"
    }
    ```


---

## cmd = MODULE_LIST
 This command is not implemented.

---

## cmd = TAG_LIST
 This command is not implemented.

---

## cmd = TAG_GET
 This command is not implemented.

---

## cmd = TAG_SET
 This command is not implemented.

---


