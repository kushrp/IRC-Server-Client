
goal: hello entry panned radio TestIRCServer timer IRCClient git

hello: hello.c
	gcc hello.c -o hello `pkg-config --cflags --libs gtk+-2.0`

panned: panned.c
	gcc panned.c -o panned `pkg-config --cflags --libs gtk+-2.0`


entry: entry.c
	gcc entry.c -o entry `pkg-config --cflags --libs gtk+-2.0`

radio: radio.c
	gcc radio.c -o radio `pkg-config --cflags --libs gtk+-2.0`

timer: timer.c
	gcc timer.c -o timer `pkg-config --cflags --libs gtk+-2.0`

TestIRCServer: TestIRCServer.c
	g++ -g -o TestIRCServer TestIRCServer.c

IRCClient: IRCClient.cc
	gcc IRCClient.cc -o IRCClient `pkg-config --cflags --libs gtk+-2.0`

git:
	git add -A  >> .local.git.out
	git commit -a -m "Make IRCclient" >> .local.git.out

clean:
	rm -f hello panned entry radio timer TestIRCServer

