const configs = {
  port: 443,
  ssl: {
    key: '/etc/ssl/snakeoil.key',
    crt: '/etc/ssl/snakeoil.crt'
  },
  users: {
    user1: 'password1',
    user2: 'password2'
  }
};

module.exports = {configs};
