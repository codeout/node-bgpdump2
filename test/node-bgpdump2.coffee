BGPDump = require('../index')
expect = require('chai').expect

describe 'node-bgpdump2', ->
  this.timeout(10000)

  beforeEach ->
    @bgpdump = new BGPDump('./test/rib.bz2')

  describe 'single IPv4 argument', ->
    it 'returns an object', ->
      result = @bgpdump.lookup('8.8.8.8')
      expect(result['prefix']).to.equal '8.8.8.0/24'
      expect(result['nexthop']).to.equal '202.249.2.169'
      expect(result['origin_as']).to.equal 15169
      expect(result['as_path']).to.equal '2497 15169'

  describe 'multiple IPv4 arguments', ->
    it 'returns an array', ->
      result = @bgpdump.lookup('8.8.8.8', '129.250.40.54')

      expect(result[0]['prefix']).to.equal '8.8.8.0/24'
      expect(result[0]['nexthop']).to.equal '202.249.2.169'
      expect(result[0]['origin_as']).to.equal 15169
      expect(result[0]['as_path']).to.equal '2497 15169'
      expect(result[1]['prefix']).to.equal '129.250.0.0/16'
      expect(result[1]['nexthop']).to.equal '202.249.2.169'
      expect(result[1]['origin_as']).to.equal 2914
      expect(result[1]['as_path']).to.equal '2497 2914'

  describe 'multiple IPv4 as an argument', ->
    it 'returns an array', ->
      result = @bgpdump.lookup(['8.8.8.8', '129.250.40.54'])

      expect(result[0]['prefix']).to.equal '8.8.8.0/24'
      expect(result[0]['nexthop']).to.equal '202.249.2.169'
      expect(result[0]['origin_as']).to.equal 15169
      expect(result[0]['as_path']).to.equal '2497 15169'
      expect(result[1]['prefix']).to.equal '129.250.0.0/16'
      expect(result[1]['nexthop']).to.equal '202.249.2.169'
      expect(result[1]['origin_as']).to.equal 2914
      expect(result[1]['as_path']).to.equal '2497 2914'

  describe 'invalid host address', ->
    it 'returns null', ->
      expect(@bgpdump.lookup('8.8.8.8/32')).to.be.null
      expect(@bgpdump.lookup('invalid')).to.be.null

  describe 'multiple arguments including invalid host address', ->
    it 'returns array including null', ->
      result = @bgpdump.lookup('8.8.8.8', '8.8.8.8/32')

      expect(result[0]).not.to.be.null
      expect(result[1]).to.be.null

  describe 'valid & invalid host address as an argument', ->
    it 'returns array including null', ->
      result = @bgpdump.lookup('8.8.8.8/32', '8.8.8.8')

      expect(result[1]).not.to.be.null
      expect(result[0]).to.be.null
