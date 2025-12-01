;;;;
;;;; server.lsp
;;;; By: tougafgc
;;;; Date: 22 November 2025
;;;;
;;;; Server that handles all processing
;;;; once connected to MBAA.exe.
;;;;

;;; DLL IMPORTS
(import "kernel32.dll" "OpenProcess")
(import "kernel32.dll" "GetCurrentProcess")
(import "kernel32.dll" "GetCurrentProcessId")
(import "kernel32.dll" "ReadProcessMemory")

(import "user32.dll" "MessageBoxA")

;;; GLOBAL VARIABLES
(define *server* 'NO-SERVER)
(define *conn*   'NO-CONNECTION)
(define *server-status* nil)

;;; FORMATTING FUNCTIONS
(define (to-hex num)
  (format "%X" num))

;;; MBAA MANIPULATION FUNCTIONS
(define (read-mem addr len)
  (let (output (pack (format "n%d" len))) ; Create a string of N null chars
    (ReadProcessMemory (GetCurrentProcess) addr (address output) len 0)
    output))

(define (write-mem addr data)
    (WriteProcessMemory (GetCurrentProcess) addr data (length data) 0))

(define (get-pid)
    (GetCurrentProcessId))

;;; SERVER FUNCTIONS
(define (start-server)
  (setq *server-status* true)
  (setq *server* (net-listen 25565 "localhost"))
  (setq *conn*   (net-accept *server*))
  (while (not (net-error))
    (net-receive *conn* buffer 1024 "\n")
    (set 'res (catch (eval-string buffer MAIN (last-error))))
    (if (last-error) (set 'res (last (last-error))))
    (net-send *conn* (append (string res) "\n")))
  (setq *server-status* nil))

(define (stop-server)
  (println "Shutting down...")
  (net-send *conn* "Shutting down...\n")
  (net-close *server*)
  (setq *server-status nil))

;;; KEYBOARD FUNCTIONS
(define (debug-msg str)
  (MessageBoxA 0 str "newLISP Debug" 0))

;; Automatically start the server on startup.
(start-server)
