<%!
  import six
%>
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//END"
 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
    <title>${name} - System info</title>
    <link rel="stylesheet" href="../result.css" type="text/css" />
  </head>
  <body>
    <h1>System info for ${name}</h1>
    <p>
      <a href="../index.html">Back to summary</a>
    </p>
    <table>
      <tr>
        <th>Detail</th>
        <th>Value</th>
      </tr>
      <tr>
        <td>totals</td>
        <td>
          <table>
            % for key, value in sorted(six.iteritems(totals), key=lambda i: (i[1], i[0]), reverse=True):
            <tr><td>${key}</td><td>${value}</td></tr>
            % endfor
            <tr><td>total</td><td>${sum(six.itervalues(totals))}</td></tr>
          </table>
        </td>
      </tr>
      <tr>
        <td>time_elapsed</td>
        <td>${time}</td>
      </tr>
      <tr>
        <td>name</td>
        <td>${name}</td>
      </tr>
      <tr>
        <td>options</td>
        <td>${options}</td>
      </tr>
      <tr>
        <td>uname -a</td>
        <td>
          <pre>${uname}</pre>
        </td>
      </tr>
      <tr>
        <td>lspci</td>
        <td>
          <pre>${lspci}</pre>
        </td>
      </tr>
      <tr>
        <td>glxinfo</td>
        <td>
          <pre>${glxinfo}</pre>
        </td>
      </tr>
      <tr>
        <td>clinfo</td>
        <td>
          <pre>${clinfo}</pre>
        </td>
      </tr>
    </table>
    <p>
      <a href="../index.html">Back to summary</a>
    </p>
  </body>
</html>
