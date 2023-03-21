import 'package:flutter/material.dart';
//import 'package:flutter_typeahead/flutter_typeahead.dart';
import 'package:fluttertoast/fluttertoast.dart';

class LogInPage extends StatefulWidget {
  const LogInPage({super.key, required this.title});
  final String title;

  @override
  State<LogInPage> createState() => _LogInPageState();
}

class _LogInPageState extends State<LogInPage> {
  final TextEditingController tEC1 = TextEditingController(),
      tEC2 = TextEditingController();
  final List<String> userid = ['masud', 'mahbub', 'system'];
  final List<String> userpsw = ['123', '123', 'abc123'];

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.start,
          children: <Widget>[
            const SizedBox(
              height: 50,
              width: 300,
            ),
            SizedBox(
              height: 50,
              width: 300,
              child: TextFormField(
                  controller: tEC1,
                  style: const TextStyle(
                    fontSize: 18,
                    color: Colors.black,
                  ),
                  decoration: const InputDecoration(
                      hintText: "User ID",
                      hintStyle: TextStyle(
                        fontSize: 18,
                        color: Colors.grey,
                      ))),
            ),
            SizedBox(
              height: 50,
              width: 300,
              child: TextFormField(
                controller: tEC2,
                style: const TextStyle(
                  fontSize: 18,
                  color: Colors.black,
                ),
                decoration: const InputDecoration(
                    hintText: "Password",
                    hintStyle: TextStyle(
                      fontSize: 18,
                      color: Colors.grey,
                    )),
                obscureText: true,
              ),
            ),
            const SizedBox(
              height: 50,
              width: 300,
            ),
            SizedBox(
                height: 40,
                width: 150,
                child: ElevatedButton(
                    style: ElevatedButton.styleFrom(
                      backgroundColor: Colors.deepPurpleAccent,
                      elevation: 5, // foreground
                    ),
                    onPressed: () {
                      if (userid.contains(tEC1.text) &&
                          userpsw[userid.indexOf(tEC1.text)] == tEC2.text) {
                        Navigator.of(context).pushNamed('hp');
                      } else {
                        Fluttertoast.showToast(
                            msg: "Invalid User ID/Password",
                            toastLength: Toast.LENGTH_SHORT,
                            gravity: ToastGravity.BOTTOM,
                            timeInSecForIosWeb: 1,
                            backgroundColor: const Color(0xFF82A30A),
                            textColor: const Color(0xFF0D1141),
                            fontSize: 16);
                      }
                    },
                    child: const Text('Login',
                        style: TextStyle(
                          fontWeight: FontWeight.w900,
                          fontSize: 20,
                          color: Colors.white,
                        )))),
          ],
        ),
      ),
    );
  }
}
