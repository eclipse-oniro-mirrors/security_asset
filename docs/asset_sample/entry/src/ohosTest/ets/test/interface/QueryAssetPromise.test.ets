import asset from '@ohos.security.asset';
import { stringToArray, removeAssetPromise, addAssetPromise, queryContentPromise, queryAssetByAlias, addAssetPromiseResult,
  removeAssetPromiseResult, queryAssetPromiseResult, postQueryAssetPromise, removeAll } from '../common'
import { describe, beforeAll, beforeEach, afterEach, afterAll, it, expect } from '@ohos/hypium';

export default function queryAssetPromiseTest() {
  describe('queryFuncPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('querySingleData', 0, async () => {
      let alias = 'querySingleData'
      expect(await addAssetPromise(alias)).assertTrue()
      let res: asset.AssetMap = await queryContentPromise(alias)
      expect(res.get(asset.Tag.SECRET)).assertDeepEquals(stringToArray('demo_pwd'))
      expect(res.get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray(alias))
      expect(await removeAssetPromise(alias)).assertTrue()
    })

    it('queryIsPersistentData', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray("queryIsPersistentTrue"))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label"))
      attr.set(asset.Tag.IS_PERSISTENT, true)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      attr.set(asset.Tag.IS_PERSISTENT, false)
      attr.set(asset.Tag.ALIAS, stringToArray("queryIsPersistentFalse"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let attr2: asset.AssetMap = new Map();
      attr2.set(asset.Tag.IS_PERSISTENT, false)
      let res = await asset.query(attr2)
      expect(res.length).assertEqual(1)
      attr2.set(asset.Tag.IS_PERSISTENT, true)
      res = await asset.query(attr2)
      expect(res.length).assertEqual(1)
      expect(await removeAll()).assertTrue()

    })

    it('queryNonExistData', 0, async () => {
      expect(await queryAssetByAlias('queryNonExistData')).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('queryMultipleData', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.ALIAS, stringToArray("queryMultipleData1"))
      attr.set(asset.Tag.SECRET, stringToArray("demo_pwd"))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray("queryMultipleData2"))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.clear()
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label"))
      let res: Array<asset.AssetMap> = await asset.query(attr)
      expect(res.length).assertEqual(2)
      expect(res[0].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray("queryMultipleData1"))
      expect(res[1].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray("queryMultipleData2"))
      expect(await removeAssetPromiseResult(attr)).assertEqual(0)
    })

    it('queryAllData', 0, async () => {
      expect(await addAssetPromise('queryAllData1')).assertTrue()
      expect(await addAssetPromise('queryAllData2')).assertTrue()

      let attr: asset.AssetMap = new Map();
      let res: Array<asset.AssetMap> = await asset.query(attr)
      expect(res.length).assertEqual(2)
      expect(res[0].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray("queryAllData1"))
      expect(res[1].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray("queryAllData2"))
      expect(await removeAssetPromiseResult(attr)).assertEqual(0)
    })

    it('queryMultipleDataNotExist', 0, async () => {
      let attr: asset.AssetMap = new Map();
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray("normal_label"))
      expect(await queryAssetPromiseResult(attr)).assertEqual(asset.ErrorCode.NOT_FOUND)
    })

    it('queryLimitLess', 0, async () => {
      expect(await addAssetPromise('queryLimitLess1')).assertTrue()
      expect(await addAssetPromise('queryLimitLess2')).assertTrue()
      expect(await addAssetPromise('queryLimitLess3')).assertTrue()

      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_LIMIT, 2);
      let res: Array<asset.AssetMap> = await asset.query(query);
      expect(res.length).assertEqual(2)
      expect(res[0].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess1'))
      expect(res[1].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess2'))
      expect(await removeAll()).assertTrue()
    })

    it('queryLimitMore', 0, async () => {
      expect(await addAssetPromise('queryLimitLess1')).assertTrue()
      expect(await addAssetPromise('queryLimitLess2')).assertTrue()
      expect(await addAssetPromise('queryLimitLess3')).assertTrue()

      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_LIMIT, 5);
      let res: Array<asset.AssetMap> = await asset.query(query);
      expect(res.length).assertEqual(3)
      expect(res[0].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess1'))
      expect(res[1].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess2'))
      expect(res[2].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess3'))
      expect(await removeAll()).assertTrue()
    })

    it('queryOffsetLess', 0, async () => {
      expect(await addAssetPromise('queryLimitLess1')).assertTrue()
      expect(await addAssetPromise('queryLimitLess2')).assertTrue()
      expect(await addAssetPromise('queryLimitLess3')).assertTrue()

      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_OFFSET, 2);
      let res: Array<asset.AssetMap> = await asset.query(query);
      expect(res.length).assertEqual(1)
      expect(res[0].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess3'))
      expect(await removeAll()).assertTrue()
    })

    it('queryOffsetMore', 0, async () => {
      expect(await addAssetPromise('queryLimitLess1')).assertTrue()
      expect(await addAssetPromise('queryLimitLess2')).assertTrue()
      expect(await addAssetPromise('queryLimitLess3')).assertTrue()

      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_OFFSET, 5);
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.NOT_FOUND)
      expect(await removeAll()).assertTrue()
    })

    it('queryOrder', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('queryLimitLess1'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('test3'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray('queryLimitLess2'))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('test2'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray('queryLimitLess3'))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('test1'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_ORDERED_BY, asset.Tag.DATA_LABEL_NORMAL_1);
      let res: Array<asset.AssetMap> = await asset.query(query);
      expect(res.length).assertEqual(3)
      expect(res[0].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess3'))
      expect(res[1].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess2'))
      expect(res[2].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess1'))
      expect(await removeAll()).assertTrue()
    })


    it('batchQuery', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('queryLimitLess1'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('test3'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray('queryLimitLess2'))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('test2'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      attr.set(asset.Tag.ALIAS, stringToArray('queryLimitLess3'))
      attr.set(asset.Tag.DATA_LABEL_NORMAL_1, stringToArray('test1'))
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_ORDERED_BY, asset.Tag.DATA_LABEL_NORMAL_1);
      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ATTRIBUTES)
      query.set(asset.Tag.RETURN_LIMIT, 1)
      query.set(asset.Tag.RETURN_OFFSET, 2)
      let res: Array<asset.AssetMap> = await asset.query(query);
      expect(res.length).assertEqual(1)
      expect(res[0].get(asset.Tag.ALIAS)).assertDeepEquals(stringToArray('queryLimitLess1'))
      expect(await removeAll()).assertTrue()
    })

    it('queryWithWrongAuthToken', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('queryWithWrongAuthToken'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)

      let query: asset.AssetMap = new Map()
      let challenge: Uint8Array = await asset.preQuery(query);
      query.set(asset.Tag.ALIAS, stringToArray('queryWithWrongAuthToken'))
      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      query.set(asset.Tag.AUTH_CHALLENGE, challenge)
      query.set(asset.Tag.AUTH_TOKEN, new Uint8Array(148))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.ACCESS_DENIED)
      expect(await postQueryAssetPromise(challenge)).assertTrue()
      expect(await removeAll()).assertTrue()
    })
  })

  describe('queryInvalidArgumentPromiseTest', () => {
    beforeAll(() => {
    })
    beforeEach(() => {
    })
    afterEach(() => {
    })
    afterAll(() => {
    })
    it('queryInvalidAliasLen', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.ALIAS, new Uint8Array(0))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.ALIAS, new Uint8Array(257))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidAccessibility', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_POWERED_ON - 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.ACCESSIBILITY, asset.Accessibility.DEVICE_UNLOCKED + 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidAuthType', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.AUTH_TYPE, asset.AuthType.NONE + 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY + 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidSyncType', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.SYNC_TYPE, asset.SyncType.NEVER - 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.SYNC_TYPE, (asset.SyncType.THIS_DEVICE | asset.SyncType.TRUSTED_DEVICE) + 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidReturnLimit', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_LIMIT, 0)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.RETURN_LIMIT, 65537)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidOrder', 0, async () => {
      let tags = [
        asset.Tag.SECRET, asset.Tag.ALIAS, asset.Tag.ACCESSIBILITY, asset.Tag.REQUIRE_PASSWORD_SET,
        asset.Tag.AUTH_TYPE, asset.Tag.SYNC_TYPE, asset.Tag.IS_PERSISTENT, asset.Tag.AUTH_VALIDITY_PERIOD,
        asset.Tag.RETURN_TYPE, asset.Tag.RETURN_LIMIT, asset.Tag.RETURN_OFFSET, asset.Tag.RETURN_ORDERED_BY,
        asset.Tag.CONFLICT_RESOLUTION, asset.Tag.AUTH_CHALLENGE, asset.Tag.AUTH_TOKEN
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(asset.Tag.RETURN_ORDERED_BY, tags[i])
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryInvalidReturnType', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL - 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ATTRIBUTES + 1)
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidChallengeLen', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.AUTH_CHALLENGE, new Uint8Array(31))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.AUTH_CHALLENGE, new Uint8Array(33))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidAuthTokenLen', 0, async () => {
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.AUTH_TOKEN, new Uint8Array(147))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

      query.set(asset.Tag.AUTH_TOKEN, new Uint8Array(149))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
    })

    it('queryInvalidLabelLen', 0, async () => {
      let labels = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4
      ]
      for (let i = 0; i < labels.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(labels[i], new Uint8Array(0))
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(labels[i], new Uint8Array(513))
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryWithoutChallenge', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('queryWithoutChallenge'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.ALIAS, stringToArray('queryWithoutChallenge'))
      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      query.set(asset.Tag.AUTH_TOKEN, new Uint8Array(148))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      expect(await removeAll()).assertTrue()
    })

    it('queryWithoutAuthToken', 0, async () => {
      let attr: asset.AssetMap = new Map()
      attr.set(asset.Tag.ALIAS, stringToArray('queryWithoutAuthToken'))
      attr.set(asset.Tag.SECRET, stringToArray('demo_pwd'))
      attr.set(asset.Tag.AUTH_TYPE, asset.AuthType.ANY)
      expect(await addAssetPromiseResult(attr)).assertEqual(0)
      let query: asset.AssetMap = new Map()
      query.set(asset.Tag.ALIAS, stringToArray('queryWithoutAuthToken'))
      query.set(asset.Tag.RETURN_TYPE, asset.ReturnType.ALL)
      query.set(asset.Tag.AUTH_CHALLENGE, new Uint8Array(32))
      expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      expect(await removeAll()).assertTrue()
    })

    it('queryBoolUnmatchedType', 0, async () => {
      let tags = [
        asset.Tag.REQUIRE_PASSWORD_SET, asset.Tag.IS_PERSISTENT
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], new Uint8Array(1))
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], 0)
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryBytesUnmatchedType', 0, async () => {
      let tags = [
        asset.Tag.DATA_LABEL_CRITICAL_1, asset.Tag.DATA_LABEL_CRITICAL_2, asset.Tag.DATA_LABEL_CRITICAL_3, asset.Tag.DATA_LABEL_CRITICAL_4,
        asset.Tag.DATA_LABEL_NORMAL_1, asset.Tag.DATA_LABEL_NORMAL_2, asset.Tag.DATA_LABEL_NORMAL_3, asset.Tag.DATA_LABEL_NORMAL_4,
        asset.Tag.AUTH_TOKEN, asset.Tag.AUTH_CHALLENGE, asset.Tag.ALIAS
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], 0)
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], false)
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryNumberUnmatchedType', 0, async () => {
      let tags = [
        asset.Tag.ACCESSIBILITY, asset.Tag.AUTH_TYPE, asset.Tag.SYNC_TYPE, asset.Tag.RETURN_LIMIT, asset.Tag.RETURN_OFFSET,
        asset.Tag.RETURN_ORDERED_BY, asset.Tag.RETURN_TYPE
      ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], new Uint8Array(1))
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)

        query.set(tags[i], false)
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })

    it('queryUnsupportedTag', 0, async () => {
      let tags = [ asset.Tag.SECRET ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(tags[i], new Uint8Array(1))
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }

      let numberTags = [ asset.Tag.AUTH_VALIDITY_PERIOD, asset.Tag.CONFLICT_RESOLUTION ]
      for (let i = 0; i < tags.length; i++) {
        let query: asset.AssetMap = new Map()
        query.set(numberTags[i], 1)
        expect(await queryAssetPromiseResult(query)).assertEqual(asset.ErrorCode.INVALID_ARGUMENT)
      }
    })
  })
}