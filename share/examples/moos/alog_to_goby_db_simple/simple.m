query = 'select * from cmoosmsg where key = "NAV_DEPTH" and double_value > 0'
database = 'simple.db';

moosmsg_nav_depth = sqlite_reader(query, database);

% for example plot depth versus time
plot(moosmsg_nav_depth.moosmsg_time-moosmsg_nav_depth.moosmsg_time(1), ...
    moosmsg_nav_depth.double_value);
axis ij; % we like to see depth = 0 at the top
title('yoyo in depth - simple.m example');
xlabel('seconds since alog start');
ylabel('depth (m)');